struct v2f
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

struct PushConstants
{
    uint direction;
    float2 texelSize;
    float pad;
};

[[vk::push_constant]]
PushConstants pc;

[[vk::combinedImageSampler]] [[vk::binding(0,0)]] Texture2D<float4> srcTex : register(t0, space0);

[[vk::combinedImageSampler]] [[vk::binding(0,0)]] SamplerState srcSampler : register(s0, space0);

float3 PrefilterBright(float3 c)
{
    const float threshold = 2.0f;
    const float knee = 1.5f;
    
    float br = max(max(c.r, c.g), c.b);
    float soft = saturate((br - threshold + knee) / (2.0f * knee));
    float contrib = max(br - threshold, 0.0f) + soft * soft * knee;
    
    return c * (contrib / max(br, 0.0001f));
}

float4 main(v2f i) : SV_Target
{
    const bool doPrefilter = (pc.direction == 0);
    
    float2 direction = (pc.direction == 0) ? float2(0.0, 1.0) : float2(1.0, 0.0);
    float2 d = direction * pc.texelSize;
    
    const float w0 = 0.227027f;
    const float w1 = 0.1945946f;
    const float w2 = 0.1216216f;
    const float w3 = 0.054054f;
    const float w4 = 0.016216f;
    
    float3 sum;
    
    float3 c0 = srcTex.Sample(srcSampler, i.uv).rgb;
    
    if (doPrefilter)
    {
        c0 = PrefilterBright(c0);
    }
    //return float4(c0, 1);
    sum = c0 * w0;
    
    float3 c1a = srcTex.Sample(srcSampler, i.uv + d * 1.0f).rgb;
    float3 c1b = srcTex.Sample(srcSampler, i.uv - d * 1.0f).rgb;
    
    if (doPrefilter)
    {
        c1a = PrefilterBright(c1a);
        c1b = PrefilterBright(c1b);
    }
    
    sum += (c1a + c1b) * w1;
    
    float3 c2a = srcTex.Sample(srcSampler, i.uv + d * 2.0f).rgb;
    float3 c2b = srcTex.Sample(srcSampler, i.uv - d * 2.0f).rgb;
    if (doPrefilter)
    {
        c2a = PrefilterBright(c2a);
        c2b = PrefilterBright(c2b);
    }
    sum += (c2a + c2b) * w2;

    float3 c3a = srcTex.Sample(srcSampler, i.uv + d * 3.0f).rgb;
    float3 c3b = srcTex.Sample(srcSampler, i.uv - d * 3.0f).rgb;
    if (doPrefilter)
    {
        c3a = PrefilterBright(c3a);
        c3b = PrefilterBright(c3b);
    }
    sum += (c3a + c3b) * w3;

    float3 c4a = srcTex.Sample(srcSampler, i.uv + d * 4.0f).rgb;
    float3 c4b = srcTex.Sample(srcSampler, i.uv - d * 4.0f).rgb;
    if (doPrefilter)
    {
        c4a = PrefilterBright(c4a);
        c4b = PrefilterBright(c4b);
    }
    sum += (c4a + c4b) * w4;

    return float4(sum, 1.0f);
}