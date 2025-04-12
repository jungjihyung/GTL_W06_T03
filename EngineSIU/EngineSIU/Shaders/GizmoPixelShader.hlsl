// GizmoPixelShader.hlsl

Texture2D<float> SceneDepth : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer MatrixConstants : register(b0)
{
    row_major float4x4 Model;
    row_major float4x4 MInverseTranspose;
    float4 UUID;
    bool isSelected;
    float3 MatrixPad0;
};

struct FMaterial
{
    float3 DiffuseColor;
    float TransparencyScalar;
    float4 AmbientColor;
    float DensityScalar;
    float3 SpecularColor;
    float SpecularScalar;
    float3 EmissiveColor;
    float MaterialPad0;
};

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

cbuffer FlagConstants : register(b2)
{
    bool IsLit;
    float3 flagPad0;
}

cbuffer CameraConstants : register(b3)
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

cbuffer ScreenConstants : register(b4)
{
    float2 ScreenSize; // 전체 화면 크기 (w, h)
    float2 UVOffset; // 뷰포트 시작 UV (x/sw, y/sh)
    float2 UVScale; // 뷰포트 크기 비율 (w/sw, h/sh)
    float2 Padding;
};


struct PS_INPUT
{
    float4 position : SV_POSITION; // 클립 공간 화면 좌표
    float3 worldPos : TEXCOORD0; // 월드 공간 위치
    float4 color : COLOR; // 전달된 베이스 컬러
    float3 normal : NORMAL; // 월드 공간 노멀
    float2 texcoord : TEXCOORD2; // UV 좌표
    int materialIndex : MATERIAL_INDEX; // 머티리얼 인덱스
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
    float4 UUID : SV_Target1;
};

// 깊이 역선형화 함수
float LinearizeDepth(float depth, float nearPlane, float farPlane)
{
    return (nearPlane * farPlane) / (depth * (farPlane - nearPlane) - farPlane);
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    output.UUID = UUID;
    
    float3 matDiffuse = Material.DiffuseColor.rgb;
    
    output.color = float4(matDiffuse, 1);
    if (isSelected)
    {
        output.color += float4(0.5f, 0.5f, 0.5f, 1); // 선택된 경우 빨간색으로 설정
    }
    
    // !TODO : SceneDepth로부터 해당 픽셀의 깊이값을 가져와서 픽셀의 깊이와 비교, 씬보다 더 깊은 경우 색을 어둡게
    // 외않되?
    // 1. UV 좌표 계산 (스크린 공간 → UV)
    float2 screenUV = input.position.xy / ScreenSize;
    screenUV = screenUV * UVScale + UVOffset;
    
    float sceneDepth = SceneDepth.Sample(LinearSampler, screenUV);
    
    // 3. 현재 픽셀의 깊이 계산 (클립 공간 Z → 선형 깊이)
    float pixelDepth = input.position.z / input.position.w;
    float linearPixelDepth = LinearizeDepth(pixelDepth, nearPlane, farPlane);
    float linearSceneDepth = LinearizeDepth(sceneDepth, nearPlane, farPlane);
    
    // 4. 픽셀이 씬보다 더 뒤에 있는 경우 색 어둡게
    if (linearPixelDepth < linearSceneDepth + 0.01f) 
    {
        output.color.rgb *= 0.3f; 
    }
    
    return output;
}
