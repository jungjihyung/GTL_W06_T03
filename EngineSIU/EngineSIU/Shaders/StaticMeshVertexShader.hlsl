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

struct VS_INPUT
{
    float3 position : POSITION; // 버텍스 위치
    float3 normal : NORMAL; // 버텍스 노멀
    float3 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR; // 버텍스 색상
    int materialIndex : MATERIAL_INDEX;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // 클립 공간으로 변환된 화면 좌표
    float3 worldPos : TEXCOORD0; // 월드 공간 위치 (조명용)
    float4 color : COLOR; // 버텍스 컬러 또는 머티리얼 베이스 컬러
    float3 normal : NORMAL; // 월드 공간 노멀
    float normalFlag : TEXCOORD1; // 노멀 유효 플래그 (1.0 또는 0.0)
    float2 texcoord : TEXCOORD2; // UV 좌표
    int materialIndex : MATERIAL_INDEX; // 머티리얼 인덱스
    float3 tangent : TANGENT; // 버텍스 탄젠트
};

#include "Light.hlsl"

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    output.materialIndex = input.materialIndex;
    
    // 월드 좌표 변환
    float4 worldPosition = mul(float4(input.position, 1.0), Model);
    output.worldPos = worldPosition.xyz;
    
    // 뷰 및 프로젝션 변환
    float4 viewPosition = mul(worldPosition, View);
    output.position = mul(viewPosition, Projection);
    
    // 월드 공간에서의 노멀 계산
    output.normal = normalize(mul(input.normal, (float3x3) MInverseTranspose));
    
    // 기본 색상 (버텍스 컬러 또는 머티리얼 베이스 컬러)
    output.color = input.color;
    
    // 텍스처 좌표 전달
    output.texcoord = input.texcoord;
    
    // 정점 쉐이더 단계에서 Gouraud 조명 계산 수행  
    // GouraudLight 함수의 반환값을 litColor에 저장하고, 이를 출력 색상에 반영
    float4 litColor = float4(1, 1, 1, 1); // 기본 색상 (조명 효과 없음)
#if LIGHTING_MODEL_GOURAUD
    litColor = CalculateTileBasedLighting(output.position.xy, worldPosition.xyz, output.normal);
#endif    
    // 원래 입력 색상과 조명 계산 결과를 곱하여 최종 색상 결정 (필요에 따라 단독 사용도 가능)
    output.color = litColor;
    
    output.tangent = input.tangent;

    return output;
}
