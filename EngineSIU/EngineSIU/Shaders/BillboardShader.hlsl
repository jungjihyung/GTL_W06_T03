Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer constants : register(b0)
{
    row_major float4x4 MVP;
    float Flag;
}

cbuffer SubUVConstant : register(b1)
{
    float2 uvOffset;
    float2 uvScale; // sub UV 셀의 크기 (예: 1/CellsPerColumn, 1/CellsPerRow)
    float4 TintColor;
}

cbuffer UUIDConstant : register(b2)
{
    float4 UUID;
}

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

PSInput MainVS(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), MVP);
    
    output.texCoord = input.texCoord;
    
    return output;
}


float4 MainPS(PSInput input) : SV_TARGET
{
    PSOutput output;
    float2 uv = input.texCoord * uvScale + uvOffset;
    float4 col = gTexture.Sample(gSampler, uv);
    float threshold = 0.1f;
    float thresholdAlpha = 0.185f;

#if DISCARD_ALPHA
    if(col.a < thresholdAlpha)
    {
        discard;
    }
    else
    {
        output.color = col;
    }
    
#else
    if (col.r < threshold && col.g < threshold && col.b < threshold)
    {
        discard;
    }
    else
    {
        output.color = col;
    }
#endif
    output.color = col * TintColor;
    
    return output.color  ;
}
