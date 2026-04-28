
struct vertexData
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
    float4 tangent : TANGENT;
};

struct v2f
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};
 
cbuffer SHADER_SCENE_DATA
{
    matrix viewMatrix, projectionMatrix, lightViewProjectionMatrices[4];
    float4 lightDirection, lightColor, ambientLightTerm, cameraWorldPosition, shadowSplits, cascadeDepthRanges, cascadeWorldUnitsPerTexel;
    uint brdfIndex, diffuseIndex, specularIndex, shadowIndex, skyboxIndex, shadowSamplerIndex;
    float skyboxRot, pad;
};

struct INSTANCE_DATA
{
    matrix worldMatrix;
    uint samplerIndex;
    uint materialIndex;
    uint pad0;
    uint pad1;
};

struct PushConstants
{
    uint index;
    float pad[31];
};

[[vk::push_constant]]
PushConstants pc;

StructuredBuffer<INSTANCE_DATA> DrawInfo : register(t1, space0);


v2f main(vertexData inputVertex, uint matrix_index : SV_InstanceID)
{
    v2f o;
    o.uv = inputVertex.uv;
    INSTANCE_DATA instance = DrawInfo[matrix_index];
    matrix rHandedWorldMatrix = mul(matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1), DrawInfo[matrix_index].worldMatrix);
    float4 posW = mul(rHandedWorldMatrix, float4(inputVertex.position, 1));
    o.position = mul(lightViewProjectionMatrices[pc.index], posW);
    return o;
}