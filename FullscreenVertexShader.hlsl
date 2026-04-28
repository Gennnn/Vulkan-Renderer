struct v2f
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

v2f main(uint vertexId : SV_VertexID)
{
    float2 p[3] =
    {
        float2(-1.0, -1.0),
        float2(3.0, -1.0),
        float2(-1.0, 3.0)
    };
    
    v2f o;
    float2 pos = p[vertexId];
    o.pos = float4(pos, 0.0, 1.0);
    
    float2 uv = pos * 0.5f + 0.5f;
    //uv.y = 1.0f - uv.y;
    o.uv = uv;
    
    return o;
}