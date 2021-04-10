#include "common.hlsl"

Texture2D    gDiffuseMap : register(t0);
Texture2D    gShadowMap : register(t1);
Texture2D    gPositionMap : register(t2);
Texture2D    gAlbedoMap : register(t3);
Texture2D    gNormalMap : register(t4);
Texture2D    gDepthMap : register(t5);

Texture3D<float4> gVoxelizerAlbedo : register(t6);
Texture3D<float4> gVoxelizerNormal : register(t7);
Texture3D<float4> gVoxelizerEmissive : register(t8);
Texture3D<float4> gVoxelizerRadiance : register(t9);
Texture3D<float4> gVoxelizerRadianceMip : register(t10);

struct VertexIn
{
    float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 Texc : TEXCOORD;
};

struct VertexOut
{
    float4 PosH  : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float3 PosW    : POSITION1;
    float3 Normal : NORMAL;
    float2 Texc : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    //// Transform to homogeneous clip space.
    ///float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    //vout.PosW = posW.xyz;
    //vout.PosH = mul(posW, gViewProj);

    //// Just pass vertex color into the pixel shader.
    //vout.Texc = vin.Texc;
    //vout.Normal = normalize(mul(vin.Normal, (float3x3)gWorld));

    //vout.ShadowPosH = mul(posW, gShadowTransform);

    vout.PosH = float4((vin.PosL) * 2.0f + float3(-1.0f , 1.0f, 0.0f), 1.0f);

    vout.Texc = vin.Texc;
    vout.Normal = vin.Normal;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    

    float4 PosW = gPositionMap.Sample(gsamLinearWrap, pin.Texc);
    float4 Alb = gAlbedoMap.Sample(gsamLinearWrap, pin.Texc);
    float4 Nor = gNormalMap.Sample(gsamLinearWrap, pin.Texc);


    int visulizevoxel = showVoxel;
    float3 voxelPickColor = float3(0.0, 0.0, 0.0);
    float4 voxelNormal = float4(0.0, 0.0, 0.0, 0.0);
    // =======================================
    // ray marched for voxel visulization
    // =======================================

    //float4 screen_space_pos = pin.PosH;
    //float sx = -(2.0 * screen_space_pos.x / gRenderTargetSize.x) + 1.0;
    //float sy =  - 1.0 + (2.0 * screen_space_pos.y / gRenderTargetSize.y);
    //float3 forward = normalize(camLookDir);
    //float3 ref = 2.0 * forward + gEyePosW;
    //float len = length(ref - gEyePosW);
    //float3 right = cross(forward, camUpDir);
    //float3 V = camUpDir * len * tan(45.0 / 2.0);
    //float3 H = right * len * (gRenderTargetSize.x / gRenderTargetSize.y) * tan(45.0 / 2.0);
    //float3 pp = ref + sx * H - sy * V;


    if (visulizevoxel) {
        float3 raydr = normalize(PosW.xyz - gEyePosW);
        float3 rayori = gEyePosW;
        float step = 0.13f;
        float3 curloc = rayori;
        int3 texDimensions;
        gVoxelizerAlbedo.GetDimensions(texDimensions.x, texDimensions.y, texDimensions.z);
        
        for (int i = 0; i < 1500; ++i) {
            float3 mappedloc = curloc / 200.0f;
            float3 texIndex = float3(((mappedloc.x * 0.5) + 0.5f),
                ((mappedloc.y * 0.5) + 0.5f) ,
                ((mappedloc.z * 0.5) + 0.5f));

            float3 texMiped = texIndex;
            texMiped.x /= 6.0;

            int3 itexIndex = int3(((mappedloc.x * 0.5) + 0.5f) * texDimensions.x,
                ((mappedloc.y * 0.5) + 0.5f) * texDimensions.y,
                ((mappedloc.z * 0.5) + 0.5f)) * texDimensions.z;

            if (visulizevoxel == 2) {
                if (all(gVoxelizerRadianceMip.SampleLevel(gsamPointClamp, texMiped, 0) == 0)) {
                    curloc = curloc + raydr * step;
                }
                else {

                    voxelNormal = gVoxelizerRadianceMip.SampleLevel(gsamPointClamp, texMiped, 0) * 2.0;
                    step = -0.1 * step;
                    curloc = curloc + raydr * step;

                }
            }
            else {
                if (all(gVoxelizerRadiance.Sample(gsamPointClamp, texIndex) == 0)) {
                    curloc = curloc + raydr * step;
                }

                else {

                    voxelPickColor = gVoxelizerRadiance.Sample(gsamPointClamp, texIndex).xyz * 1.0 ;
                    voxelNormal = gVoxelizerRadiance.Sample(gsamPointClamp, texIndex).xyzw ;
                    step = -0.1 * step;
                    curloc = curloc + raydr * step;

                }
            }



        }
    }


    // vxgi
    float3 up = float3(0, 1, 0);
    float3 right = cross(Nor.xyz, up);
    float3 forward = cross(Nor.xyz, right);
    float diffuseRadiusRatio = tan(0.53);

    float3 diffuseCol = float3(0.0, 0.0, 0.0);
    float diffusOcclusion = 0.0;
    float startSampledDis = 0.2;

    for (int conei = 0; conei < 6; ++conei) {
        float3 coneSamplePos = PosW.xyz;
        coneSamplePos += Nor.xyz * VOXELSCALE * 1.0;
        float3 originalSamplePos = coneSamplePos;

        float3 sampleDir = Nor.xyz;
        sampleDir += cVXGIConeSampleDirections[conei].x * right + cVXGIConeSampleDirections[conei].z * forward;
        sampleDir = normalize(sampleDir);

        coneSamplePos += sampleDir * startSampledDis;
        float curRadius = diffuseRadiusRatio * startSampledDis;
        float curDis = startSampledDis;

        float3 accDiffuseCol = float3(0, 0, 0);
        float accDiffusOcclusion = 0.0;
        float ambientOCcAcc = 0.0;

        for (int i = 0; i < 64; ++i) {
            bool outSideVol = false;
            float4 sampleCol = sampleVoxelVolumeAnisotropic(
                gVoxelizerRadiance,
                gVoxelizerRadianceMip,
                gsamAnisotropicClamp,
                coneSamplePos,
                curRadius,
                sampleDir,
                outSideVol
                );
            float lastDis = curDis;
            curDis = curDis / (1.0 - diffuseRadiusRatio);
            coneSamplePos = originalSamplePos + sampleDir * curDis;
            curRadius = diffuseRadiusRatio * curDis;

            sampleCol.a = 1.0 - pow(abs(1.0 - sampleCol.a), ((curDis - lastDis) / curDis));
            accumulateColorOcclusion(sampleCol, accDiffuseCol, accDiffusOcclusion);

            ambientOCcAcc += sampleCol.a * (0.1 / (curDis + 1.0));

            if (accDiffusOcclusion >= 0.99f || outSideVol) break;
        }
        diffuseCol += accDiffuseCol * cVXGIConeSampleWeights[conei];

    }


    // =======================================
    // deferred shadow mapping
    // =======================================

    float4 shadowPosH = mul(PosW, gShadowTransform);
    float3 shadowPosNDC = shadowPosH.xyz / shadowPosH.w;
    float percentLit = 0.0f;
    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);
    // Texel size.
    float dx = 1.0f / (float)width;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        float2 curPos = shadowPosNDC.xy + offsets[i];
        if (gShadowMap.Sample(gsamLinearWrap, curPos).r > shadowPosNDC.z) {
            percentLit += 0.1f;
        }
    }
    //percentLit += 0.1f;

    float3 lightDir = gLightPosW;
    lightDir = normalize(lightDir);
    float lamb = dot(lightDir, Nor);

    float4 col = float4(gAlbedoMap.Sample(gsamLinearWrap, pin.Texc).rgb, 1.0f);
    col = col * lamb * percentLit;
    if (visulizevoxel == 1) {
        col = float4(voxelPickColor, 1.0f);
    }
    else if (visulizevoxel == 2) {
        col = float4(voxelNormal.xyz, 1.0f);
    }
    col += float4(diffuseCol * Alb.xyz/1.0, 0.0);
    //col = float4(diffuseCol, 1.0);
    return col;
}


