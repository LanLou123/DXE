


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    float gObj2VoxelScale;
    int IsDynamic;
};

#define VOXELSCALE 1.5
#define VOXELMIPCOUNT 9
#define PI 3.1415926

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gShadowTransform;
    float3 gEyePosW;
    float cbPerObjectPad0;
    float3 gLightPosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4x4 gVoxelView;
    float4x4 gVoxelProj;
    float4x4 gVoxelViewProj;
    float3 camLookDir;
    float cbPerObjectPad2;
    float3 camUpDir;
    int showVoxel;
    int showDirect;
};


cbuffer cbMaterial : register(b2)
{
    float4   gDiffuseAlbedo;
    float3   gFresnelR0;
    float    gRoughness;
    float4x4 gMatTransform;
    int      IsEmissive;
};

float4 convRGBA8ToVec4(uint val)
{
    float4 re = float4(float((val & 0x000000FF)), float((val & 0x0000FF00) >> 8U), float((val & 0x00FF0000) >> 16U), float((val & 0xFF000000) >> 24U));
    return clamp(re, float4(0.0,0.0,0.0,0.0), float4(255.0,255.0,255.0,255.0));
}

uint convVec4ToRGBA8(float4 val)
{
    return (uint (val.w) & 0x000000FF) << 24U | (uint(val.z) & 0x000000FF) << 16U | (uint(val.y) & 0x000000FF) << 8U | (uint(val.x) & 0x000000FF);
}


//analytic bayer over 2 domains, is unrolled loop of GetBayerFromCoordLevel().
//but in terms of reusing subroutines, which is faster,while it does not extend as nicely.
float bayer2(float2 a) { a = floor(a); return frac(dot(a, float2(.5, a.y * .75))); }
float bayer4(float2 a) { return bayer2(.5 * a) * .25 + bayer2(a); }
float bayer8(float2 a) { return bayer4(.5 * a) * .25 + bayer2(a); }
float bayer16(float2 a) { return bayer4(.25 * a) * .0625 + bayer4(a); }
float bayer32(float2 a) { return bayer8(.25 * a) * .0625 + bayer4(a); }
float bayer64(float2 a) { return bayer8(.125 * a) * .015625 + bayer8(a); }
float bayer128(float2 a) { return bayer16(.125 * a) * .015625 + bayer8(a); }
#define dither2(p)   (bayer2(  p)-.375      )
#define dither4(p)   (bayer4(  p)-.46875    )
#define dither8(p)   (bayer8(  p)-.4921875  )
#define dither16(p)  (bayer16( p)-.498046875)
#define dither32(p)  (bayer32( p)-.499511719)
#define dither64(p)  (bayer64( p)-.49987793 )
#define dither128(p) (bayer128(p)-.499969482)

static const float3 ConeSampleDirections[] =
{
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.5f, 0.866025f),
    float3(0.823639f, 0.5f, 0.267617f),
    float3(0.509037f, 0.5f, -0.7006629f),
    float3(-0.50937f, 0.5f, -0.7006629f),
    float3(-0.823639f, 0.5f, 0.267617f)
};

float getMipLevelFromRadius(float radius)
{
    return max(log2(float(radius + 0.0f) / VOXELSCALE) - 0.0, 0.0);
}

static const float cMipDirectionalOffsets[] = {
    0.0f,
    1.0f / 6.0f,
    2.0f / 6.0f,
    3.0f / 6.0f,
    4.0f / 6.0f,
    5.0f / 6.0f
};

static const float ConeSampleWeights[] =
{
    PI / 4.0f,
    3 * PI / 20.0f,
    3 * PI / 20.0f,
    3 * PI / 20.0f,
    3 * PI / 20.0f,
    3 * PI / 20.0f,
};

//static const float ConeSampleWeights[] =
//{
//    0.25,
//    0.15,
//    0.15,
//    0.15,
//    0.15,
//    0.15,
//};

void accumulateColorOcclusion(float4 sampleColor, inout float3 colorAccum, inout float occlusionAccum)
{
    colorAccum = occlusionAccum * colorAccum + (1.0f - occlusionAccum) * sampleColor.a * sampleColor.rgb;
    occlusionAccum = occlusionAccum + (1.0f - occlusionAccum) * sampleColor.a;
}


float4 sampleVoxelVolumeAnisotropic(Texture3D<float4> voxelTexture, Texture3D<float4> voxelMips, SamplerState voxelSampler, float3 worldPosition, float radius, float3 direction, inout bool outsideVolume)
{
    //direction = -direction;
    //uint3 isNegative = (direction < 0.0f);
    uint3 isNegative;
    isNegative.x = (direction.x < 0.0f) ? 1 : 0;
    isNegative.y = (direction.y < 0.0f) ? 1 : 0;
    isNegative.z = (direction.z < 0.0f) ? 1 : 0;

    float3 dirSq = direction * direction;

    float3 voxelPos = worldPosition / 200.0; // 200 as placeholder
    voxelPos = voxelPos * 0.5f + 0.5f;

    float anisotropicMipLevel = getMipLevelFromRadius(radius);

   
    float4 filteredColor = float4(0.0, 0.0, 0.0, 0.0);
    //if (anisotropicMipLevel > -1.0) {
        if (any(voxelPos.xyz < 0.0f) || any(voxelPos.xyz > 1.0f))
            outsideVolume = true;

        voxelPos.x /= 6.0f;

        float4 xSample = voxelMips.SampleLevel(voxelSampler, voxelPos + float3(cMipDirectionalOffsets[isNegative.x], 0.0f, 0.0f), anisotropicMipLevel);
        float4 ySample = voxelMips.SampleLevel(voxelSampler, voxelPos + float3(cMipDirectionalOffsets[isNegative.y + 2], 0.0f, 0.0f), anisotropicMipLevel);
        float4 zSample = voxelMips.SampleLevel(voxelSampler, voxelPos + float3(cMipDirectionalOffsets[isNegative.z + 4], 0.0f, 0.0f), anisotropicMipLevel);

        filteredColor = dirSq.x * xSample + dirSq.y * ySample + dirSq.z * zSample;

        //float4 xSample0 = voxelMips.SampleLevel(voxelSampler, voxelPos, anisotropicMipLevel);
        //filteredColor = xSample0;
   // }


    filteredColor.rgb *= 1.0f * pow(anisotropicMipLevel, 0.0);
 
    //filteredColor.a *= 0.8;

    return filteredColor;
}
