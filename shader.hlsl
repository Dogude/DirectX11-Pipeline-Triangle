// Texture
Texture2D ColorTexture : register(t0);
SamplerState ColorSampler : register(s0);

// Matris buffer (CPU -> GPU)
cbuffer TransformBuffer : register(b0)
{
    float4x4 WorldViewProjection;
}

// Input/Output
struct VS_INPUT
{
    float3 ObjectPosition : POSITION;
    float2 TextureCoordinate : TEXCOORD;
    float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 TextureCoordinate : TEXCOORD;
};
    
VS_OUTPUT VSMain(VS_INPUT IN)
{
    VS_OUTPUT OUT;

    OUT.Position = mul(float4(IN.ObjectPosition, 1.0f), WorldViewProjection);
    OUT.TextureCoordinate = IN.TextureCoordinate;

    return OUT;
}

float4 PSMain(VS_OUTPUT IN) : SV_Target
{
    return ColorTexture.Sample(ColorSampler, IN.TextureCoordinate);
}
