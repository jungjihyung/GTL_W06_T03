Texture2D SceneDepth : register(t0);
SamplerState Sampler : register(s0);

cbuffer ConstantBuffer : register(b1)
{
    row_major float4x4 InvViewProj;
    float4 FogColor;
    float3 CameraPos;
    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;
    float FogMaxOpacity;
    float3 FogPosition;
    float CameraNear;
    float CameraFar;
};

cbuffer ScreenConstants : register(b0)
{
    uint2 ScreenSize; // 전체 화면 크기 (w, h)
    float2 UVOffset; // 뷰포트 시작 UV (x/sw, y/sh)
    float2 UVScale; // 뷰포트 크기 비율 (w/sw, h/sh)
    float2 Padding;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD;
};

float4 mainPS(PS_INPUT input) : SV_Target
{
    // 텍스처 좌표 보정
    float2 TextureUV = input.UV * UVScale + UVOffset;

    // 장면 깊이 샘플 및 NDC z값 계산
    float rawDepth = SceneDepth.Sample(Sampler, TextureUV).r;
    float z_ndc = rawDepth * 2.0f - 1.0f;
        
    // 선형 깊이 계산
    float linearZ = CameraNear * CameraFar / (CameraFar - rawDepth * (CameraFar - CameraNear));

    // NDC 좌표 계산 및 view ray 복원
    float2 ndc = float2(input.UV.x * 2.0f - 1.0f, 1.0f - input.UV.y * 2.0f);
    float4 clipPos = float4(ndc.x, ndc.y, 1.0f, 1.0f);
    float4 viewRay4 = mul(clipPos, InvViewProj);
    float3 viewRay = normalize(viewRay4.xyz - CameraPos);
    
    // 픽셀의 월드 좌표 재구성
    float3 worldPos = CameraPos + viewRay * linearZ;

    // 카메라와 픽셀 사이의 거리 (안개 효과의 기본 요소)
    float distance = max(length(worldPos - CameraPos), 1e-2f);

    // 높이 감쇠: 높이차에 따른 안개 감쇠. FogHeightFalloff가 0이면 항상 1이 됨.
    float heightDiff = abs(worldPos.z - FogPosition.z);
    float heightAtten = (FogHeightFalloff > 0) ? exp(-FogHeightFalloff * heightDiff) : 1.0f;

    // 거리 기반 exponential fog 효과 계산
    float fogFactorExp = 1.0f - exp(-FogDensity * distance);
    float fogFactor = fogFactorExp * heightAtten;

    // StartDistance와 FogCutoffDistance 구간에서 선형 보간 적용하여 안개 효과 점진적 적용
    float lerpFactor = saturate((distance - StartDistance) / max(FogCutoffDistance - StartDistance, 1e-5f));
    fogFactor = lerp(0.0f, fogFactor, lerpFactor);

    // 최대 안개 불투명도 제한
    fogFactor = min(fogFactor, FogMaxOpacity);

   
    // 최종 색상 계산: fog 색상을 fogFactor로 조절하여 출력
    float4 finalColor = float4(FogColor.rgb * fogFactor, fogFactor);
    return finalColor;
}
