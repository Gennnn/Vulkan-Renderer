
struct v2f
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(v2f input) : SV_TARGET
{
    return 0;
}