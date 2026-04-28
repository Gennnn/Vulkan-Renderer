
struct v2f
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

v2f main(uint i : SV_VertexID)
{
    float2 p;
    if (i == 0)
        p = float2(-1, -1);
    if (i == 1)
        p = float2(-1, 3);
    if (i == 2)
        p = float2(3, -1);
    
    v2f o;
    o.pos = float4(p, 0, 1);
    o.uv = p;
    return o;
}