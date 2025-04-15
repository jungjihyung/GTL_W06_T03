#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 256
#define MAX_LIGHTS 256

struct LIGHT
{
    float3 m_cBaseColor;
    float pad3;

    float3 m_vPosition;
    float m_fFalloff; // 스팟라이트의 감쇠 인자 (스포트 각도에 따른 감쇠)

    float3 m_vDirection;
    float pad4;

    float m_fAttenuation; // 거리 기반 감쇠 계수
    int m_bEnable;
    int m_nType;
    float m_fIntensity; // 광원 강도
    
    float m_fAttRadius; // 감쇠 반경 (Attenuation Radius)
    float3 LightPad;
};

struct TileFrustum
{
    float4 planes[4]; // left, right, top, bottom
};

cbuffer MatrixConstants : register(b0)
{
    row_major float4x4 Model;
    row_major float4x4 MInverseTranspose;
    float4 UUID;
    bool isSelected;
    float3 MatrixPad0;
};
cbuffer CameraConstants : register(b1)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    row_major float4x4 InvProjection;

    float3 CameraPosition;
    float cameraPad1;
    
    float nearPlane;
    float farPlane;
    float2 cameraPad2;
};
cbuffer ScreenConstants : register(b2)
{
    uint2 ScreenSize; // 전체 화면 크기 (w, h)
    float2 UVOffset; // 뷰포트 시작 UV (x/sw, y/sh)
    float2 UVScale; // 뷰포트 크기 비율 (w/sw, h/sh)
    float2 Padding;
};
cbuffer cbLights : register(b3)
{
    //LIGHT gLights[MAX_LIGHTS];
    float4 gcGlobalAmbientLight;
    uint gnLights;
    float3 padCB;
};

Texture2D<float> depthTexture : register(t0);
StructuredBuffer<LIGHT> gLights : register(t1); // 광원 정보

RWStructuredBuffer<uint> visibleLightIndices : register(u0);    // 출력 버퍼
RWBuffer<uint> lightIndexCount : register(u1);

groupshared uint sharedLightIndices[MAX_LIGHTS_PER_TILE];   // 현재 타일의 Frustum이 영향받는 Light들의 인덱스
groupshared uint sharedLightCount;                          // 현재 타일의 Frustum이 영향받는 Light들의 개수
groupshared uint sharedMinDepth;                            // 현재 타일의 Frustum의 최소 깊이
groupshared uint sharedMaxDepth;                            // 현재 타일의 Frustum의 최대 깊이

TileFrustum ComputeTileFrustum(uint2 tileID, float2 invTileCount)
{
    TileFrustum frustum;
    
    float2 tileScale = float2(invTileCount) * 2.0f;
    
    float2 ndcMin = -1.0 + tileID * tileScale;
    float2 ndcMax = ndcMin + tileScale;
    
    float4 leftPlaneNDC = float4(1, 0, 0, -ndcMin.x);
    float4 rightPlaneNDC = float4(-1, 0, 0, ndcMax.x);
    float4 bottomPlaneNDC = float4(0, 1, 0, -ndcMin.y);
    float4 topPlaneNDC = float4(0, -1, 0, ndcMax.y);
    
    
    float4x4 invProj = InvProjection;
    
    frustum.planes[0] = mul(leftPlaneNDC, invProj); // 왼쪽
    frustum.planes[1] = mul(rightPlaneNDC, invProj); // 오른쪽
    frustum.planes[2] = mul(topPlaneNDC, invProj); // 위쪽
    frustum.planes[3] = mul(bottomPlaneNDC, invProj); // 아래쪽
    
    return frustum;
    
}

float LinearizeDepth(float depth, float near, float far)
{
    return (2.0 * near * far) / (far + near - depth * (far - near));
}

bool LightIntersectTile(LIGHT light, TileFrustum frustum, float minDepth, float maxDepth)
{
    if(light.m_bEnable == 0)
        return false;
    
    if(light.m_nType == 3) // directional light
        return true;
    
    // 광원의 위치를 view 공간으로 변환
    float3 lightPosViewSpace = mul(float4(light.m_vPosition, 1.0f), View).xyz;
    
    // 광원의 영향 반경
    float radius = light.m_fAttRadius * saturate(light.m_fIntensity / light.m_fAttenuation);
    
    // 1. 타일 평면과의 충돌 검사
    for (uint i = 0; i < 4; ++i)
    {
        float distance = dot(frustum.planes[i].xyz, lightPosViewSpace) + frustum.planes[i].w;
        if(distance > radius) 
            return false;
    }
    
    // 2. 깊이 검사
    float lightMinDepth = lightPosViewSpace.z - radius;
    float lightMaxDepth = lightPosViewSpace.z + radius;
    
    if (lightMaxDepth < minDepth || lightMinDepth > maxDepth)
        return false;
    
    return true;
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void mainCS(
    uint3 groupID : SV_GroupID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // 1. 타일 정보 계산
    uint2 tileID = groupID.xy;
    uint2 pixelCoord = tileID * TILE_SIZE + groupThreadID.xy;
    float2 tileCount = float2(
        ceil(ScreenSize.x / (float) TILE_SIZE),
        ceil(ScreenSize.y / (float) TILE_SIZE));
    
    float2 invTileCount = 1.0f / tileCount;
    
    // 2. 깊이 버퍼에서 최소/최대 깊이 계산 ->
    bool bValidPixel = (dispatchThreadID.x <= ScreenSize.x && dispatchThreadID.y <= ScreenSize.y);
    float depth = bValidPixel ? depthTexture.Load(int3(pixelCoord, 0)).r : 1.0f; // 유효하지 않으면 1.0 반환
    
    //float depth = depthTexture.Load(int3(pixelCoord, 0)).r;
    float ndcDepth = depth * 2.0 - 1.0;

    float linearDepth = LinearizeDepth(ndcDepth, nearPlane, farPlane);
    
    // 3. 공유 메모리 초기화(첫 스레드에서만)
    if (groupThreadID.x == 0 && groupThreadID.y == 0)
    {
        sharedLightCount = 0;
        
        sharedMinDepth = asuint(1e30f); // 아주 큰 값
        sharedMaxDepth = asuint(0.0f); // 아주 작은 값
        
        // 광원 인덱스 배열 초기화
        for (uint i = 0; i < MAX_LIGHTS_PER_TILE; ++i)
        {
            sharedLightIndices[i] = 0;
        }
    }
    
    // 4. 모든 스레드 동기화될 때까지 대기
    GroupMemoryBarrierWithGroupSync();
    
    // 5. 원자적 최소/최대 깊이 갱신
    InterlockedMin(sharedMinDepth, asuint(linearDepth));
    InterlockedMax(sharedMaxDepth, asuint(linearDepth));
    
    GroupMemoryBarrierWithGroupSync();
    
    if (groupThreadID.x == 0 && groupThreadID.y == 0)
    {
        float minDepth = asfloat(sharedMinDepth);
        float maxDepth = asfloat(sharedMaxDepth);
        
        TileFrustum frustum = ComputeTileFrustum(tileID, invTileCount);
        // 5. 각 광원에 대한 충돌 검사
        for (uint lightIdx = 0; lightIdx < gnLights; ++lightIdx)
        {
            LIGHT light = gLights[lightIdx];
        
            if (LightIntersectTile(light, frustum, minDepth, maxDepth))
            {
                uint dstIdx;
                InterlockedAdd(sharedLightCount, 1, dstIdx);
            
                if (dstIdx < MAX_LIGHTS_PER_TILE)
                {
                    sharedLightIndices[dstIdx] = lightIdx;
                }
            }
        }
    }
    
    // 스레드동기화
    GroupMemoryBarrierWithGroupSync();
    


    // 결과를 전역 메모리에 저장
    if(groupThreadID.x == 0 && groupThreadID.y == 0)
    {
        uint tilesX = (ScreenSize.x + TILE_SIZE - 1) / TILE_SIZE;
        //uint tilesY = (ScreenSize.y + TILE_SIZE - 1) / TILE_SIZE;
        
        uint tileIndex = tileID.y * tilesX + tileID.x;
        uint baseIndex = tileIndex * MAX_LIGHTS_PER_TILE;
        
        uint lightCount = min(sharedLightCount, MAX_LIGHTS_PER_TILE);
        lightIndexCount[tileIndex] = lightCount;
        
        // 가시광원 인덱스 저장
        for (uint i = 0; i < sharedLightCount && i < MAX_LIGHTS_PER_TILE; ++i)
        {
            visibleLightIndices[baseIndex + i] = sharedLightIndices[i];
        }
    }
}
