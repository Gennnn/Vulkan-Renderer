

struct v2f
{
    float4 pos : SV_POSITION;
    float3 nrm : NORMAL;
    float2 uv : UV;
    float4 tangent : TANGENT;
    float3 posW : WORLD;
    nointerpolation uint instanceID : TEXCOORD3;
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

static const float PI = 3.141592;
static const float Epsilon = 0.00001;
static const float3 Fdielectric = 0.04;
static const float4x4 biasMatrix = float4x4(
0.5, 0, 0, 0.5,
0, 0.5, 0, 0.5,
0 , 0, 1,   0,
0, 0, 0,    1
);

StructuredBuffer<INSTANCE_DATA> DrawInfo : register(t1, space0);
StructuredBuffer<MATERIAL_DATA> MaterialInfo : register(t2, space0);


Texture2D textures[] : register(t0, space1);
TextureCube cubeTextures[] : register(t0, space2);
SamplerState samplers[] : register(s0, space1);

float ComputeCascadeBias(uint cascadeIndex, float cosLi)
{
    float depthRange = cascadeDepthRanges[cascadeIndex];
    float texelWU = cascadeWorldUnitsPerTexel[cascadeIndex];

    float slopeTerm = (1.0f - cosLi); 
    
    float worldBias = texelWU * lerp(1.5f, 4.0f, slopeTerm);
    worldBias = max(worldBias, 0.005);
    return worldBias / depthRange;

}

float TextureProj(float4 sc, float2 offset, uint cascadeIndex, float bias)
{
    float2 uv = sc.xy + offset;
    bool occluded = false;
    if (sc.z > 0 && sc.z < 1)
    {
        uint shadowInd = NonUniformResourceIndex(shadowIndex + cascadeIndex);
        float storedDepth = textures[shadowInd].SampleLevel(samplers[shadowSamplerIndex], uv, 0).r;
        if (sc.w > 0 && storedDepth > sc.z + bias)
        {
            occluded = true;
        }
    }
    return occluded ? ambientLightTerm.x : 1.0f;
}

float FilterPCF(float4 sc, uint cascadeIndex, float cosLi)
{
    uint shadowInd = NonUniformResourceIndex(shadowIndex + cascadeIndex);
    int2 dim;
    textures[shadowInd].GetDimensions(dim.x, dim.y);

    const float scale = 0.75;
    const int range = 2;
    

    float2 texel = 1.0 / float2(dim);
    float2 margin = texel * (range + 0.5f);
    
    float2 halfTex = texel * 0.5f;
    float2 kernelUV = texel * (range * scale);

    float bias = 0.005;
    
    float sum = 0.0f;
    int count = 0;

    for (int x = -range; x <= range; x++)
        for (int y = -range; y <= range; y++)
        {
            float2 uvOff = float2(x, y) * texel * scale;
            sum += TextureProj(sc, uvOff, cascadeIndex, bias);
            count++;
        }
    return sum / (float) count;
}

float ndfGGX(float cosLh, float roughness)
{
    roughness = max(roughness, 0.04f);
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    denom = max(denom, 1e-6f);
    return alphaSq / (PI * denom * denom);
}

float gaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

float3 fresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 LinearToSRGB(float3 x)
{
    x = saturate(x);
    float3 lo = x * 12.92;
    float3 hi = 1.055 * pow(x, 1.0 / 2.4) - 0.055;
    float3 useHi = step(0.0031308, x); // 0 if x < edge, else 1
    return lerp(lo, hi, useHi);
}

float4 main(v2f input) : SV_TARGET
{
    //return float4(1, 1, 1, 1);
    //return float4(normalize(input.nrm) * 0.5f + 0.5f, 1);
    uint samplerIndex = DrawInfo[input.instanceID].samplerIndex;
    uint materialIndex = DrawInfo[input.instanceID].materialIndex;
    MATERIAL_DATA matData = MaterialInfo[materialIndex];
    float4 col;
    
    uint nIdx = NonUniformResourceIndex(matData.normalIndex);
    uint bcIdx = NonUniformResourceIndex(matData.baseColorIndex);
    uint mrIdx = NonUniformResourceIndex(matData.metallicRoughnessIndex);
    
    float3 nrmTangent = normalize(input.tangent.xyz);
    float3 nrm = normalize(input.nrm);
    float3 binormal = normalize(cross(nrm, nrmTangent));
    binormal *= input.tangent.w;
    float3x3 tbn = float3x3(nrmTangent, binormal, nrm);
    float3 nrmCol = textures[nIdx].Sample(samplers[samplerIndex], input.uv).rgb;
    //return float4(nrmCol, 1);
    nrmCol.g = 1.0f - nrmCol.g;
    nrmCol = nrmCol * 2.0f - 1.0f;
    nrmCol = normalize(mul(nrmCol, tbn));
    //return float4(nrmCol * 0.5f + 0.5f, 1);
    
    float4 rmaoSample = textures[mrIdx].Sample(samplers[samplerIndex], input.uv);

    float ambientOcclusion = 1;
    float roughness = rmaoSample.g;
    roughness = max(roughness, 0.04f);
    float metallic = rmaoSample.b;
    
    float3 directLight = 0.0f;
    float3 indirectLight = 0.0f;
    
    float4 baseSample = textures[bcIdx].Sample(samplers[samplerIndex], input.uv);
    float4 baseColor = baseSample * matData.baseColorFactor;
    float3 baseCol = baseColor.rgb;
    float alpha = baseColor.a;
    if (matData.alphaMode == 1 && alpha < matData.alphaCutoff)
    {
        discard;
    }
    float3 f0 = lerp(Fdielectric, baseCol, metallic);
    float3 diffuseColor = baseCol * (1.0f - metallic);
    
    float3 nrmLightDir = normalize(lightDirection.xyz);
    float3 viewDir = normalize(cameraWorldPosition.xyz - input.posW);
    float3 halfVector = normalize(-nrmLightDir.xyz + viewDir);
    
    float cosLi = max(0.0, dot(nrmCol, -nrmLightDir.xyz));
    float cosLh = max(0.0, dot(nrmCol, halfVector));
    float cosLo = max(0.0, dot(nrmCol, viewDir));
    float3 F = fresnelSchlick(f0, max(0.0, dot(halfVector, viewDir)));
    float D = ndfGGX(cosLh, roughness);
    float G = gaSchlickGGX(cosLi, cosLo, roughness);
    
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
    
    float3 diffuseBRDF = kd * baseCol / PI;

    float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

    directLight += (diffuseBRDF + specularBRDF) * lightColor.xyz * cosLi;
    
    
    float3 irradiance = cubeTextures[diffuseIndex].Sample(samplers[samplerIndex], nrmCol).rgb;
    float3 f1 = fresnelSchlick(f0, cosLo);

    kd = lerp(1.0 - f1, 0.0, metallic);

    float3 diffuseIBL = kd * baseCol.xyz / PI * irradiance;
    
    uint width, height, specularTextureLevels;
    
    cubeTextures[specularIndex].GetDimensions(0, width, height, specularTextureLevels);
		// Sample pre-filtered specular reflection environment at correct mipmap level.
    float mip = roughness * (specularTextureLevels - 1);
    mip = clamp(mip, 0.0, (float) (specularTextureLevels - 1));
    float3 Lr = 2.0 * cosLo * nrmCol - viewDir;
    
    float3 specularIrradiance = cubeTextures[specularIndex].SampleLevel(samplers[samplerIndex], Lr, mip).rgb;
    float2 brdf = textures[brdfIndex].Sample(samplers[samplerIndex], float2(cosLo, roughness)).rg;
    float3 specularIBL = (f0 * brdf.x + brdf.y) * specularIrradiance;
    indirectLight = diffuseIBL * ambientOcclusion + specularIBL;
    
    uint cascadeIndex = 0;
    float4 viewPos = mul(viewMatrix, float4(input.posW, 1));
    
    float depthForCascades = viewPos.z;
    float eps = max(1e-3f, 0.5f * max(abs(ddx(depthForCascades)), abs(ddy(depthForCascades))));
    for (uint i = 0; i < 3; ++i)
    {
        
        if (depthForCascades >= shadowSplits[i] - eps )
        {
            cascadeIndex = i + 1;
        }
    }
    float4 shadowCoordinate = mul(biasMatrix, mul(lightViewProjectionMatrices[cascadeIndex], float4(input.posW, 1)));
    float3 nrmGeom = normalize(input.nrm);
    float cosLiGeom = max(0.0, dot(nrmGeom, -nrmLightDir.xyz));
    float shadow = FilterPCF(shadowCoordinate, cascadeIndex, cosLiGeom);
    float3 lit = directLight * shadow + indirectLight;
    if (any(isnan(lit)) || any(isinf(lit)))
        lit = 0;
    col = float4(lit, alpha);
    
    //+textures[3].Sample(samplers[3], input.uv);
    /*
    switch (cascadeIndex)
    {
        case 0:
            col.rgb *= float3(1, 0.25, 0.25);
            break;
        case 1:
            col.rgb *= float3(0.25, 1, 0.25);
            break;
        case 2:
            col.rgb *= float3(0.25, 0.25, 1);
            break;
        case 3:
            col.rgb *= float3(1, 1, 0.25);
            break;
    }
*/
    
    
    
    return col;
}