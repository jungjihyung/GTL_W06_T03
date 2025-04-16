#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 256
#define MAX_LIGHTS 8192

Buffer<uint> LightIndexCount : register(t3);

cbuffer ScreenConstants : register(b2)
{
    uint2 ScreenSize; // 전체 화면 크기 (w, h)
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

float3 HeatMapColor(float t)
{
    if(t == 0)
        return float3(0, 0, 0);
    if (t < 0.33)
    {
        // Blue to Green
        float k = t / 0.33;
        return lerp(float3(0, 0, 1), float3(0, 1, 0), k);
    }
    else if (t < 0.66)
    {
        // Green to Yellow
        float k = (t - 0.33) / 0.33;
        return lerp(float3(0, 1, 0), float3(1, 1, 0), k);
    }
    else
    {
        // Yellow to Red
        float k = (t - 0.66) / 0.34;
        return lerp(float3(1, 1, 0), float3(1, 0, 0), k);
    }
}

PS_OUTPUT mainPS(VS_OUTPUT input)
{
    PS_OUTPUT output;
    uint2 screenPos = input.position.xy;

    uint tilesPerRow = (ScreenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    
    uint tileX = screenPos.x / TILE_SIZE;
    uint tileY = screenPos.y / TILE_SIZE;
    
    uint tileIndex = tileY * tilesPerRow + tileX;
    
    uint lightCount = LightIndexCount.Load(tileIndex);
    
    float maxHeat = 25.0f;
    float intensity = saturate(lightCount / maxHeat);
  
    float2 tileLocal;
    tileLocal.x = fmod(screenPos.x, TILE_SIZE);
    tileLocal.y = fmod(screenPos.y, TILE_SIZE);
    
    // 5. 타일의 중심과 반지름 설정  
    float2 center = float2(TILE_SIZE * 0.5, TILE_SIZE * 0.5);
    float radius = TILE_SIZE * 0.5;
   
    float dist = length(tileLocal - center);
    float mask = smoothstep(radius, radius - 2.f, dist);
   
    output.color = float4(HeatMapColor(intensity), mask);
    return output;
}
