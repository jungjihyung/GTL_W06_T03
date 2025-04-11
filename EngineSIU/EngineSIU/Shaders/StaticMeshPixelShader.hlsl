// staticMeshPixelShader.hlsl

Texture2D Textures : register(t0);
SamplerState Sampler : register(s0);

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
    float3 CameraPosition;
    float pad;
};

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
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
    float4 UUID : SV_Target1;
};


PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    output.UUID = UUID;

    // 1) 알베도 샘플링
    float3 albedo = Textures.Sample(Sampler, input.texcoord).rgb;
    // 2) 머티리얼 디퓨즈
    float3 matDiffuse = Material.DiffuseColor.rgb;
    // 3) 라이트 계산

    bool hasTexture = any(albedo != float3(0, 0, 0));
    
    float3 baseColor = hasTexture ? albedo : matDiffuse;

#if LIT_MODE    
        float3 lightRgb = Lighting(input.worldPos, input.normal).rgb;
        float3 litColor = baseColor * lightRgb;
        output.color = float4(litColor, 1);
#else
    output.color = float4(baseColor, 1); 
#endif

#if IsSelected
        output.color += float4(0.02, 0.02, 0.02, 1);
#endif
    return output;
}
