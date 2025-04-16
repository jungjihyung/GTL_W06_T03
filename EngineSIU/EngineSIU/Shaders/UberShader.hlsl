// UberShader.hlsl
#include "MVPShader.hlsl"
/////////////////////////////////////////////////////////////
// 텍스처 및 샘플러
/////////////////////////////////////////////////////////////
Texture2D DiffuseMap : register(t0);
#if HAS_NORMAL_MAP
Texture2D NormalMap : register(t1);
#endif
SamplerState Sampler : register(s0);

/////////////////////////////////////////////////////////////
// 머티리얼 구조체 및 상수 버퍼
/////////////////////////////////////////////////////////////
struct FMaterial
{
    float3 DiffuseColor;
    float TransparencyScalar;
    
    float3 AmbientColor;
    float DensityScalar;
    
    float3 SpecularColor;
    float SpecularScalar;
    
    float3 EmissiveColor;
    float MaterialPad0;
};

cbuffer MaterialConstants : register(b3)
{
    FMaterial Material;
}

/////////////////////////////////////////////////////////////
// 서브메쉬 상수 (선택 상태 등)
/////////////////////////////////////////////////////////////
cbuffer SubMeshConstants : register(b4)
{
    bool IsSelectedSubMesh;
    float3 SubMeshPad0;
}

/////////////////////////////////////////////////////////////
// 텍스처 좌표 관련 상수 버퍼
/////////////////////////////////////////////////////////////
cbuffer TextureConstants : register(b5)
{
    float2 UVOffset;
    float2 TexturePad0;
}

/////////////////////////////////////////////////////////////
// 스크린 관련 상수 버퍼
/////////////////////////////////////////////////////////////
cbuffer ScreenConstants : register(b6)
{
    uint2 ScreenSize; // 전체 화면 크기 (w, h)
    float2 ScreenUVOffset; // 뷰포트 시작 UV (x/sw, y/sh)
    float2 UVScale; // 뷰포트 크기 비율 (w/sw, h/sh)
    float2 Padding;
}

/////////////////////////////////////////////////////////////
// 라이팅 관련 정의
/////////////////////////////////////////////////////////////
#define MAX_LIGHTS 256
#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 256

#define POINT_LIGHT         1
#define SPOT_LIGHT          2
#define DIRECTIONAL_LIGHT   3

struct LIGHT
{
    float3 m_cBaseColor;
    float pad3;

    float3 m_vPosition;
    float m_fFalloff; // 스팟라이트 감쇠 인자

    float3 m_vDirection;
    float pad4;

    float m_fAttenuation; // 거리 기반 감쇠 계수
    float m_fIntensity; // 광원 강도
    float m_fAttRadius; // 감쇠 반경
    float m_fInnerConeAngle;
    
    float m_fOuterConeAngle;
    float m_Falloff;
    float2 m_OuterConeAnglePad;
    
    int m_bEnable;
    int m_nType;
    int2 TypePad;
};

StructuredBuffer<uint> VisibleLightIndices : register(t2);
Buffer<uint> LightIndexCount : register(t3);
StructuredBuffer<LIGHT> gLights : register(t4);

cbuffer cbLights : register(b2)
{
    float4 gcGlobalAmbientLight;
    int gnLights;
    float3 padCB;
}


/////////////////////////////////////////////////////////////
// 라이팅 함수들
/////////////////////////////////////////////////////////////
float4 CalcLight(int nIndex, float3 vPosition, float3 vNormal)
{
    float4 litColor = float4(0, 0, 0, 1);
    
    if (!gLights[nIndex].m_bEnable)
    {
        return litColor;
    }
    
    if (gLights[nIndex].m_nType == POINT_LIGHT)
    {
        float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
        float fDistance = length(vToLight);

        // 감쇠 반경을 벗어나면 기여하지 않음
        if (fDistance > gLights[nIndex].m_fAttRadius)
        {
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance; // 정규화
        
        float fDiffuseFactor = max(dot(vNormal, vToLight), 0);
        

        float3 litResult = float3(0, 0, 0);
        float fAttenuationFactor = 1.0f / (1.0f + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
   
#if LIGHTING_MODEL_LAMBERT 
        litResult  = gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor;
#else   
        if (fDiffuseFactor > 0.0f)
        {
            float3 vView = normalize(CameraPosition - vPosition);
            float3 vHalf = normalize(vToLight + vView);
            fSpecularFactor = pow(max(dot(normalize(vNormal), vHalf), 0.0f), 1); 
        }

    
        litResult = (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor) +
                 (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor);
#endif
        return float4(litResult * fAttenuationFactor * gLights[nIndex].m_fIntensity, 1.0f);
    }
    
    else if (gLights[nIndex].m_nType == SPOT_LIGHT)
    {
        float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
        float fDistance = length(vToLight);
        if (fDistance < gLights[nIndex].m_fAttRadius)
        {
            vToLight /= fDistance;
            float fDiffuseFactor = saturate(dot(vNormal, vToLight));
            float fSpecularFactor = 0.0;
            if (fDiffuseFactor > 0.0)
            {
                float3 vView = normalize(CameraPosition - vPosition);
                float3 vHalf = normalize(vToLight + vView);
                fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
            }
            
            float fAngleCos = dot(-vToLight, normalize(gLights[nIndex].m_vDirection));
            float cosInner = cos(gLights[nIndex].m_fInnerConeAngle * 0.0174533f);
            float cosOuter = cos(gLights[nIndex].m_fOuterConeAngle * 0.0174533f);
            
            if (fAngleCos > 0.0)
            {
                float fSpotFactor = pow(fAngleCos, gLights[nIndex].m_fFalloff);
                float fAttenuationFactor = 1.0 / (1.0 + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
                
                litColor = float4(
                    (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb) +
                    (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor.rgb),
                    1.0
                );
                litColor *= fAttenuationFactor * fSpotFactor * gLights[nIndex].m_fIntensity;
            }
        }
    }
    else if (gLights[nIndex].m_nType == DIRECTIONAL_LIGHT)
    {
        float3 lightDir = normalize(-gLights[nIndex].m_vDirection);
        float fDiffuseFactor = saturate(dot(vNormal, lightDir));
        float fSpecularFactor = 0.0;
        if (fDiffuseFactor > 0.0)
        {
            float3 vView = normalize(CameraPosition - vPosition);
            float3 vHalf = normalize(lightDir + vView);
            fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
            fSpecularFactor *= fDiffuseFactor;
        }
        
        litColor = float4(
            gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb +
            gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor.rgb,
            1.0
        );
    }
    
    return litColor;
}

float4 CalculateTileBasedLighting(uint2 screenPos, float3 worldPos, float3 normal)
{
    float4 result = gcGlobalAmbientLight;
    
    // 픽셀의 타일 id 계산
    uint tilesPerRow = (ScreenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    uint tileX = screenPos.x / TILE_SIZE;
    uint tileY = screenPos.y / TILE_SIZE;
    uint tileIndex = tileY * tilesPerRow + tileX;
    
    // 해당 타일의 가시 광원 수 조회
    uint lightCount = LightIndexCount.Load(tileIndex);
    lightCount = min(lightCount, MAX_LIGHTS_PER_TILE);
    
    uint baseIndex = tileIndex * MAX_LIGHTS_PER_TILE;
    for (uint i = 0; i < lightCount; ++i)
    {
        uint lightIdx = VisibleLightIndices.Load(baseIndex + i);
        result += CalcLight(lightIdx, worldPos, normal);
    }
    
    result += gcGlobalAmbientLight;
    result.a = 1.0;
    
    return result;
}

float4 Lighting(float3 vPosition, float3 vNormal)
{
    float4 cColor = float4(gcGlobalAmbientLight.rgb * Material.AmbientColor.rgb, 1.0f);
    [unroll(16)]
    for (int i = 0; i < gnLights; i++)
    {
        cColor += CalcLight(i, vPosition, vNormal);
    }
    cColor += gcGlobalAmbientLight;
    cColor.a = 1.0;
    
    return cColor;
}

/////////////////////////////////////////////////////////////
// 구조체 정의 (정점 및 픽셀)
/////////////////////////////////////////////////////////////
struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
    int materialIndex : MATERIAL_INDEX;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // 클립 공간 좌표
    float3 worldPos : TEXCOORD0; // 월드 좌표 (라이팅용)
    float4 color : COLOR; // 베이스 컬러 (버텍스 컬러 또는 머티리얼 색상)
    float3 normal : NORMAL; // 월드 좌표 노멀
    float normalFlag : TEXCOORD1; // 노멀 유효 플래그
    float2 texcoord : TEXCOORD2; // UV 좌표
    int materialIndex : MATERIAL_INDEX;
    float3 tangent : TANGENT; // 탄젠트
};

/////////////////////////////////////////////////////////////
// 정점 쉐이더
/////////////////////////////////////////////////////////////
PS_INPUT MainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.materialIndex = input.materialIndex;
    
    // 월드 변환
    float4 worldPosition = mul(float4(input.position, 1.0), Model);
    output.worldPos = worldPosition.xyz;
    
    // 뷰 및 프로젝션 변환
    float4 viewPosition = mul(worldPosition, View);
    output.position = mul(viewPosition, Projection);
    
    // 월드 공간 노멀 계산
    output.normal = normalize(mul(input.normal, (float3x3) MInverseTranspose));
    
    // 기본 색상 전달 (버텍스 컬러)
    output.color = input.color;
    
    // 텍스처 좌표 전달
    output.texcoord = input.texcoord;
    
    // 라이팅을 위한 계산 (필요시 Gouraud 모델 사용)
#if LIGHTING_MODEL_GOURAUD
    float4 litColor = CalculateTileBasedLighting(output.position.xy, worldPosition.xyz, output.normal);
    output.color = litColor;
#endif

    output.tangent = input.tangent;
    
    return output;
}

/////////////////////////////////////////////////////////////
// 픽셀 쉐이더
/////////////////////////////////////////////////////////////
struct PS_OUTPUT
{
    float4 color : SV_Target0;
};

PS_OUTPUT MainPS(PS_INPUT input)
{
    PS_OUTPUT output;

    // 알베도 및 머티리얼 디퓨즈 결정
    float3 albedo = DiffuseMap.Sample(Sampler, input.texcoord).rgb;
    float3 matDiffuse = Material.DiffuseColor.rgb;
    float3 baseColor = any(albedo != float3(0, 0, 0)) ? albedo : matDiffuse;
    
    // 노멀 매핑 처리
    float3 normal;
#if HAS_NORMAL_MAP
        float3 sampledNormal = NormalMap.Sample(Sampler, input.texcoord);
        float gamma = 1.0 / 2.2;
        sampledNormal = pow(sampledNormal, gamma) * 2.0 - 1.0;
        float3 T = normalize(input.tangent);
        float3 N = normalize(input.normal);
        T = normalize(T - dot(T, N) * N);
        float3 B = normalize(cross(T, N));
        float3x3 TBN = float3x3(T, B, N);
        normal = normalize(mul(sampledNormal, TBN));
#else
        normal = input.normal;
#endif
    
    // 라이팅 계산 (조건에 따라 다른 모드를 선택)
#if LIT_MODE
#if LIGHTING_MODEL_GOURAUD
        float3 lightRgb = input.color.rgb;
#else
        float3 lightRgb = CalculateTileBasedLighting(input.position.xy, input.worldPos, normal).rgb;
#endif
    float3 litColor = baseColor * lightRgb;
    output.color = float4(litColor, 1.0);
#elif WORLD_NORMAL_MODE
    output.color = float4(normal * 0.5 + 0.5, 1.0);
#else
    output.color = float4(baseColor, 1.0);
#endif

    if (IsSelectedSubMesh)
    {
        output.color += float4(0.02, 0.02, 0.02, 1.0);
    }
    
    return output;
}
