
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
    float3 normal : NORMAL;
    float2 uv : UV;
    float4 tangent : TANGENT;
    float3 wPosition : WORLD;
    nointerpolation uint instanceID : TEXCOORD3;
};
 
cbuffer SHADER_SCENE_DATA
{
    matrix viewMatrix, projectionMatrix, lightViewProjectionMatrices[4];
    float4 lightDirection, lightColor, ambientLightTerm, cameraWorldPosition, shadowSplits, cascadeDepthRanges, cascadeWorldUnitsPerTexel;
    uint brdfIndex, diffuseIndex, specularIndex, shadowIndex, skyboxIndex, shadowSamplerIndex;
    float skyboxRot;
};

struct INSTANCE_DATA
{
    matrix worldMatrix;
    uint samplerIndex;
    uint materialIndex;
    uint pad0;
    uint pad1;
};


struct MATERIAL_DATA
{
    float4 baseColorFactor;
    uint normalIndex;
    uint baseColorIndex;
    uint metallicRoughnessIndex;
    uint alphaMode;
    float alphaCutoff;
    float transmissionFactor;
    float pad[2];
};

StructuredBuffer<INSTANCE_DATA> DrawInfo : register(t1, space0);
StructuredBuffer<MATERIAL_DATA> MaterialInfo : register(t2, space0);

v2f main(vertexData inputVertex, uint matrix_index : SV_InstanceID)
{
    v2f o;
    
    matrix rHandedWorldMatrix = mul( matrix(1,0,0,0,0,1,0,0,0,0,-1,0,0,0,0,1), DrawInfo[matrix_index].worldMatrix );
    float4 worldPosition = mul(rHandedWorldMatrix, float4(inputVertex.position, 1));
    o.wPosition = worldPosition.xyz;
    o.position = mul(projectionMatrix, mul(viewMatrix, mul(rHandedWorldMatrix, float4(inputVertex.position, 1))));
    float3x3 world3x3 = (float3x3) rHandedWorldMatrix;
    o.normal = normalize(mul(world3x3, inputVertex.normal));
    o.uv = inputVertex.uv;
    float w = inputVertex.tangent.w;
    o.tangent.xyz = normalize(mul(world3x3, inputVertex.tangent.xyz));
    o.tangent.w = w;
    o.instanceID = matrix_index;
    return o;
}