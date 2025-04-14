#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 256
#define MAX_LIGHTS 256

StructuredBuffer<uint> VisibleLightIndices : register(t2);
Buffer<uint> LightIndexCount : register(t3);

cbuffer ScreenConstants : register(b2)
{
    float2 ScreenSize; // 전체 화면 크기 (w, h)
    float2 ScreenUVOffset; // 뷰포트 시작 UV (x/sw, y/sh)
    float2 UVScale; // 뷰포트 크기 비율 (w/sw, h/sh)
    float2 Padding;
};

// 입력 구조체: POSITION과 TEXCOORD
struct VS_INPUT
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

// 출력 구조체
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};


struct PS_OUTPUT
{
    float4 color : SV_Target0;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = float4(input.position.xy, 1.0f, 1.0f);
    output.texCoord = input.texCoord;
    
    return output;
}


PS_OUTPUT mainPS(VS_OUTPUT input)
{
    uint2 screenPos = (uint2) (input.position.xy);
    uint2 tilecoord = (screenPos / TILE_SIZE);
    uint tilesPerRow = (uint) (ScreenSize.x / TILE_SIZE);
    uint tileIndex = tilecoord.y * tilesPerRow + tilecoord.x;
    
    uint lightCount = LightIndexCount[tileIndex];
    
    float intensity = saturate(lightCount / 10.0f);
    PS_OUTPUT output;
    output.color = float4(0, intensity, 0, 1);
    return output;
}

