
struct v2f
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

cbuffer SHADER_SCENE_DATA
{
    matrix viewMatrix, projectionMatrix, lightViewProjectionMatrices[4];
    float4 lightDirection, lightColor, ambientLightTerm, cameraWorldPosition, shadowSplits, cascadeDepthRanges, cascadeWorldUnitsPerTexel;
    uint brdfIndex, diffuseIndex, specularIndex, shadowIndex, skyboxIndex, shadowSamplerIndex;
    float skyboxRot;
};

Texture2D textures[] : register(t0, space1);
TextureCube cubeTextures[] : register(t0, space2);
SamplerState samplers[] : register(s0, space1);

float3 Rotate(float3 d, float yaw)
{
    float s, c;
    sincos(-yaw, s, c);
    
    return float3(c * d.x + s * d.z, d.y, -s * d.x + c * d.z);
}

float4 main(v2f i) : SV_Target
{
    float m00 = projectionMatrix[0][0];
    float m11 = projectionMatrix[1][1];
    
    float3 dirVS = normalize(float3(i.uv.x / m00, i.uv.y / m11, 1));
    
    float3x3 invViewRot = transpose((float3x3) viewMatrix);
    float3 dirWS = mul(invViewRot, dirVS);
    dirWS = Rotate(dirWS, skyboxRot);
    
    float3 sky = cubeTextures[skyboxIndex].SampleLevel(samplers[0], dirWS, 0).rgb;
    return float4(sky, 1.0);
}