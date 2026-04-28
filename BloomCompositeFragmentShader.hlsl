struct v2f
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

[[vk::combinedImageSampler]] [[vk::binding(0,0)]] 
Texture2D<float4> sceneTex : register(t0, space0);

[[vk::combinedImageSampler]] [[vk::binding(0,0)]] 
SamplerState sceneSampler : register(s0, space0);

[[vk::combinedImageSampler]] [[vk::binding(1,0)]] 
Texture2D<float4> bloomTex : register(t1, space0);

[[vk::combinedImageSampler]] [[vk::binding(1,0)]] 
SamplerState bloomSampler : register(s1, space0);

float3 Reinhard(float3 x)
{
    return x / ( 1.0f + x);
}

float4 main(v2f i) : SV_Target
{
    
    float3 scene = sceneTex.Sample(sceneSampler, i.uv).rgb;
    float3 bloom = bloomTex.Sample(bloomSampler, i.uv).rgb;
    
    const float bloomStrength = 3.0f;
    float3 color = scene + bloomStrength * bloom;
    
    return float4(color, 1.0f);
}