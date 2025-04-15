// DepthDebug.hlsl
#define CAMERA_NEAR 0.1f
#define CAMERA_FAR  100.0f

Texture2D DepthTexture : register(t0);
SamplerState DepthSampler : register(s0);

cbuffer ScreenConstants : register(b0)
{
    uint2 ScreenSize; // 전체 화면 크기 (w, h)
    float2 UVOffset; // 뷰포트 시작 UV (x/sw, y/sh)
    float2 UVScale; // 뷰포트 크기 비율 (w/sw, h/sh)
    float2 Padding;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 mainPS(PS_INPUT IN) : SV_TARGET
{
    // 1) 뷰포트별 UV 계산
    float2 uv = IN.uv * UVScale + UVOffset;

    // 2) 원시 깊이 샘플링
    float raw = DepthTexture.Sample(DepthSampler, uv).r;

    // 3) NDC → 선형 깊이 변환
    float z_ndc = raw * 2.0 - 1.0;
    float lin = (2.0 * CAMERA_NEAR * CAMERA_FAR)
                  / (CAMERA_FAR + CAMERA_NEAR - z_ndc * (CAMERA_FAR - CAMERA_NEAR));

    // 4) 0~1 정규화
    float norm = saturate((lin - CAMERA_NEAR) / (CAMERA_FAR - CAMERA_NEAR));

    return float4(norm, norm, norm, 1.0);
}

//float4 mainPS(PS_INPUT IN) : SV_Target
//{
//    // 1) 원본 깊이 읽기 (0~1)
//    float raw = DepthTexture.Sample(DepthSampler, IN.uv).r;

//    float minDepth = 0.995;
//    float maxDepth = 1.f;
//    float v = smoothstep(minDepth, maxDepth, raw);

//    // 3) 흑백 출력
//    return float4(v, v, v, 1.0);
//}
