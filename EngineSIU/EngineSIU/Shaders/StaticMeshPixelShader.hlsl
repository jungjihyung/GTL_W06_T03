// staticMeshPixelShader.hlsl

Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);
SamplerState Sampler : register(s0);

#include "MVPShader.hlsl"

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

cbuffer SubMeshConstants : register(b4)
{
    bool IsSelectedSubMesh;
    float3 SubMeshPad0;
}

cbuffer TextureConstants : register(b5)
{
    float2 UVOffset;
    float2 TexturePad0;
}

#include "Light.hlsl"

struct PS_INPUT
{
    float4 position : SV_POSITION; // 클립 공간 화면 좌표
    float3 worldPos : TEXCOORD0; // 월드 공간 위치
    float4 color : COLOR; // 전달된 베이스 컬러
    float3 normal : NORMAL; // 월드 공간 노멀
    float normalFlag : TEXCOORD1; // 노멀 유효 플래그
    float2 texcoord : TEXCOORD2; // UV 좌표
    int materialIndex : MATERIAL_INDEX; // 머티리얼 인덱스
    float3 tangent : TANGENT; // 버텍스 탄젠트
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
};


PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    
    // 1) 알베도 샘플링
    float3 albedo = DiffuseMap.Sample(Sampler, input.texcoord).rgb;
    // 2) 머티리얼 디퓨즈
    float3 matDiffuse = Material.DiffuseColor.rgb;
    // 3) 라이트 계산

    // 4) 노멀 샘플링
    float3 sampledNormal = NormalMap.Sample(Sampler, input.texcoord).xyz;
    
    bool hasTexture = any(albedo != float3(0, 0, 0));
    
    float3 baseColor = hasTexture ? albedo : matDiffuse;
    
    float3 normal;
    
    if (length(sampledNormal))
    {
        float3x3 TBN = float3x3(input.tangent, cross(input.normal, input.tangent), input.normal);
        normal = normalize(mul(sampledNormal, TBN));
    }
    else
        normal = input.normal;
#if LIT_MODE    

    #if LIGHTING_MODEL_GOURAUD
        float3 litColor = baseColor * input.color;
        output.color = float4(litColor, 1);
    #else
        float3 lightRgb = Lighting(input.worldPos, normal).rgb;
        float3 litColor = baseColor * lightRgb;
        output.color = float4(litColor, 1);
    #endif    

#elif WORLD_NORMAL_MODE
        output.color = float4(normal * 0.5 + 0.5, 1.0);
#else
    output.color = float4(baseColor, 1.0);
#endif
    

    if (isSelected)
        output.color += float4(0.02, 0.02, 0.02, 1);


    return output;
}
