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
    row_major float4x4 InvProjection;
    float3 CameraPosition;
    float pad;
    float CameraNear;
    float CameraFar;
    float campad[2];
};
