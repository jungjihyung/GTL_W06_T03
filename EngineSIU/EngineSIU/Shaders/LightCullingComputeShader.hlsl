#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 256
#define MAX_LIGHTS 8192

struct LIGHT
{
    float3 m_cBaseColor;
    float pad3;

    float3 m_vPosition;
    float m_fFalloff; // 스팟라이트의 감쇠 인자 (스포트 각도에 따른 감쇠)

    float3 m_vDirection;
    float pad4;

    float m_fAttenuation; // 거리 기반 감쇠 계수
    float m_fIntensity; // 광원 강도
    float m_fAttRadius; // 감쇠 반경 (Attenuation Radius)
    float m_InnerConeAngle;
    
    float m_OuterConeAngle;
    float m_Falloff;
    float2 m_OuterConeAnglePad;
    
    int m_bEnable;
    int m_nType;
    int2 m_Pad;
};

struct TileFrustum
{
    float4 planes[4]; // left, right, top, bottom
    float3 min;
    float3 max;
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

//groupshared uint sharedLightIndices[MAX_LIGHTS_PER_TILE];   // 현재 타일의 Frustum이 영향받는 Light들의 인덱스
//groupshared uint sharedLightCount;                          // 현재 타일의 Frustum이 영향받는 Light들의 개수
groupshared uint sharedMinDepth;                            // 현재 타일의 Frustum의 최소 깊이
groupshared uint sharedMaxDepth;                            // 현재 타일의 Frustum의 최대 깊이

float LinearizeDepth(float depth, float near, float far)
{
    return (near * far) / (far - depth * (far - near));
}

float3 GetViewRay(float2 pixel)
{
    float2 ndc = (pixel / ScreenSize) * 2.0f - 1.0f;
    ndc.y = -ndc.y; // DirectX 좌표계 맞춤

    float4 clip = float4(ndc, 1.0f, 1.0f); // z = 1: far plane
    float4 view = mul(clip, InvProjection); // row-major
    return normalize(view.xyz / view.w);
}

bool RayIntersectsSphere(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float radius)
{
    float3 oc = rayOrigin - sphereCenter;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    return h >= 0;
}

bool LightIntersectTile(LIGHT light, uint2 tileID, float minDepth, float maxDepth)
{
    if (light.m_nType == 3)
        return true;
    
    float3 camPos = float3(0, 0, 0);// view공간이기 때문
    float radius = light.m_fAttRadius;
    
    // 1. Light 위치를 view 공간으로 변환
    float4 lightPosViewSpace = mul(float4(light.m_vPosition, 1.0f), View);
    float3 lightPos = lightPosViewSpace.xyz;
    
    
    float lightMinDepth = lightPos.z - radius;
    float lightMaxDepth = lightPos.z + radius;
    
    if (lightMaxDepth < minDepth || lightMinDepth > maxDepth)
        return false;
    
    // directional일 경우 true

    
    // 1. 타일 꼭짓점 좌표
    float2 pixelUL = tileID * TILE_SIZE;
    float2 pixelUR = (tileID + float2(1, 0)) * TILE_SIZE;
    float2 pixelLL = (tileID + float2(0, 1)) * TILE_SIZE;
    float2 pixelLR = (tileID + float2(1, 1)) * TILE_SIZE;
    
    
    // 2. View 공간 Ray 계산
    float3 rayUL = GetViewRay(pixelUL);
    float3 rayUR = GetViewRay(pixelUR);
    float3 rayLL = GetViewRay(pixelLL);
    float3 rayLR = GetViewRay(pixelLR);
    

    if (RayIntersectsSphere(camPos, rayUL, lightPos, radius))
        return true;
    
    if (RayIntersectsSphere(camPos, rayUR, lightPos, radius))
        return true;
    
    if (RayIntersectsSphere(camPos, rayLL, lightPos, radius))
        return true;
    
    if (RayIntersectsSphere(camPos, rayLR, lightPos, radius))
        return true;
    
    return false;
}


[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void mainCS(
    uint3 groupID : SV_GroupID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint3 dispatchThreadID : SV_DispatchThreadID,
    uint groupIndex : SV_GroupIndex)
{
    // 1. 타일 정보 계산
    uint2 tileID = groupID.xy;
    uint2 pixelCoord = tileID * TILE_SIZE + groupThreadID.xy;
    float2 tileCount = float2(
        ceil(ScreenSize.x / (float) TILE_SIZE),
        ceil(ScreenSize.y / (float) TILE_SIZE));
    
    float2 invTileCount = 1.0f / tileCount;
    
    // 2. 깊이 버퍼에서 최소/최대 깊이 계산 ->
    //bool bValidPixel = (dispatchThreadID.x <= ScreenSize.x && dispatchThreadID.y <= ScreenSize.y);
    bool bValidPixel = (dispatchThreadID.x < ScreenSize.x && dispatchThreadID.y < ScreenSize.y);

    float depth = bValidPixel ? depthTexture.Load(int3(pixelCoord, 0)).r : 1.0f; // 유효하지 않으면 1.0 반환
    
    //float depth = depthTexture.Load(int3(pixelCoord, 0)).r;
    float ndcDepth = depth * 2.0 - 1.0;

    float linearDepth = LinearizeDepth(depth, nearPlane, farPlane);
    
    // 3. 공유 메모리 초기화(첫 스레드에서만)
    if (groupThreadID.x == 0 && groupThreadID.y == 0)
    {
        sharedMinDepth = asuint(1e30f); // 아주 큰 값
        sharedMaxDepth = asuint(0.0f); // 아주 작은 값

    }
    
    // 4. 모든 스레드 동기화될 때까지 대기
    GroupMemoryBarrierWithGroupSync();
    
    // 5. 원자적 최소/최대 깊이 갱신
    InterlockedMin(sharedMinDepth, asuint(linearDepth));
    InterlockedMax(sharedMaxDepth, asuint(linearDepth));
    
    GroupMemoryBarrierWithGroupSync();
    float minDepth = asfloat(sharedMinDepth);
    float maxDepth = asfloat(sharedMaxDepth);
    
    uint tilesX = (ScreenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    uint tileIndex = tileID.y * tilesX + tileID.x;
    
  
    uint totalThreads = TILE_SIZE * TILE_SIZE;

    for (uint i = groupIndex; i < gnLights; i += totalThreads)
    {
        LIGHT light = gLights[i];
        if (LightIntersectTile(light, tileID, minDepth, maxDepth))
        {
            uint dstIdx;
            InterlockedAdd(lightIndexCount[tileIndex], 1, dstIdx); // 원자적 증가
            if (dstIdx < MAX_LIGHTS_PER_TILE)
            {
                visibleLightIndices[tileIndex * MAX_LIGHTS_PER_TILE + dstIdx] = i; // groupIndex가 아닌 i를 할당
            }
        }
    }
}
