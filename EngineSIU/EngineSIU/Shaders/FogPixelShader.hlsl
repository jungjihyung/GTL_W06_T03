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
    float2 TextureUV = input.UV * UVScale + UVOffset;
    float2 UV = input.UV;
    
    //픽셀 월드 위치 계산
    float rawDepth = SceneDepth.Sample(Sampler, TextureUV).r;
    float z_ndc = rawDepth * 2.0f - 1.0;  
    float linearZ = CameraNear * CameraFar / (CameraFar - rawDepth * (CameraFar - CameraNear));
    if (rawDepth == 1)
        linearZ = CameraNear * 100.0 / (100.0 - rawDepth * (100.0 - CameraNear));

    // NDC 기준 ray 방향 (정규화된 뷰 방향)
        float2 ndc = float2(UV.x * 2.0 - 1.0, 1.0 - UV.y * 2.0);
    float4 clip = float4(ndc.x, ndc.y, 1.0, 1.0);
    float4 viewRay4 = mul(clip, InvViewProj);
    float3 viewRay = normalize(viewRay4.xyz - CameraPos);

    // 픽셀의 world 위치 재구성
    float3 worldPos = CameraPos + viewRay * linearZ;
    
    //높이 감쇠
    float HeightDiff = max(worldPos.z - FogPosition.z, 1e-4);
    
    // 카메라 ~ 안개 중심 거리 기반 감쇠
    float CameraHeightDiff = max(CameraPos.z - FogPosition.z, 1e-4);
   
     //카메라 까지의 거리 계산
    float distance = length(worldPos - CameraPos);
    distance = max(distance, 1e-2f);
    
    //안개 까지의 거리 계산
    float FogDistance = length(worldPos - FogPosition);
    
    //안개 계수
    float FogFactor = exp((-FogHeightFalloff * HeightDiff));
    float CameraFogFactor = exp((-FogHeightFalloff * CameraHeightDiff));
    
    float TotalFogFactor = TotalFogFactor = FogDensity * (CameraFogFactor + FogFactor) / (max(FogHeightFalloff * distance, 1e-5));

    TotalFogFactor = saturate(TotalFogFactor);
    TotalFogFactor = min(TotalFogFactor, FogMaxOpacity);
    
    float FadeRange = 50.0f;
    float t = smoothstep(StartDistance, StartDistance + FadeRange, distance);
    TotalFogFactor *= t;
    
    if (distance < StartDistance)
        TotalFogFactor = 0;
    
    float4 FinalColor = float4(FogColor.rgb * TotalFogFactor, TotalFogFactor);
    return FinalColor;
}
