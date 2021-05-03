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
            texMiped.x += 1.0 / 6.0;

            int3 itexIndex = int3(((mappedloc.x * 0.5) + 0.5f) * texDimensions.x,
                ((mappedloc.y * 0.5) + 0.5f) * texDimensions.y,
                ((mappedloc.z * 0.5) + 0.5f)) * texDimensions.z;

            if (visulizevoxel == 2) {
                if (all(gVoxelizerRadianceMip.SampleLevel(gsamPointClamp, texMiped, 2) == 0)) {
                    curloc = curloc + raydr * step;
                }
                else {

                    voxelNormal = gVoxelizerRadianceMip.SampleLevel(gsamPointClamp, texMiped, 2) * 1.0;
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
    //if (abs(dot(Nor.xyz, up)) == 1.0) {
    //    up = float3(0.0, 0.0, 1.0);
    //}
    //float3 vright = cross(Nor.xyz, up);
    //if (abs(dot(Nor.xyz, up)) == 1.0) {
    //    up = float3(0, 0, -1);
    //}
     float3 vright = cross(Nor.xyz, up);
    float3 vforward = cross(Nor.xyz, vright);

    float aperture = 0.577;

    float3 diffuseCol = float3(0.0, 0.0, 0.0);
    float diffusOcclusion = 0.0;
    float startSampledDis = VOXELSCALE;

 
    for (int conei = 0; conei < 6; ++conei) {



        float3 sampleDir = Nor.xyz;
        float3 curSampleDir = ConeSampleDirections[conei];
        //curSampleDir = normalize(curSampleDir);
        sampleDir += curSampleDir.x * vright + curSampleDir.z * vforward;
        sampleDir = normalize(sampleDir);


        float dst = startSampledDis;
        float3 startSamplePos = PosW.xyz + Nor.xyz * dst;


 
        float VA = 0.0;
        float3 VC = float3(0, 0, 0);
        float4 VC4 = float4(0, 0, 0,0);

        float curRadius = aperture * dst * 2.0;

        for (int i = 0; i < 64; ++i) {
            bool outSideVol = false; 
            
            float3 coneSamplePos = startSamplePos + sampleDir * dst;
            curRadius = aperture * dst * 2.0;

            float4 sampleCol = sampleVoxelVolumeAnisotropic(
                gVoxelizerRadiance,
                gVoxelizerRadianceMip,
                gsamAnisotropicClamp,
                coneSamplePos,
                curRadius,
                sampleDir,
                outSideVol
                );



           // accDiffuseCol += (1.0 - accDiffuseCol.a) * sampleCol;




            //VC = VC * VA + (1 - VA) * sampleCol.rgb * sampleCol.a;
            //VA = VA + (1 - VA) * sampleCol.a ;


            VC4 += (1 - VC4.a) * sampleCol;
            VA += (1 - VA) * sampleCol.a;// / (1.0 + curRadius * 0.0);


            //sampleCol.a = 1.0 - pow(abs(1.0 - sampleCol.a), ((curDis - lastDis) / curDis));
            //accumulateColorOcclusion(sampleCol, accDiffuseCol, accDiffusOcclusion);

            //ambientOCcAcc += sampleCol.a * ((1.0 - ambientOCcAcc) / (curRadius * 0.6 + 1.0));

            if (VA >= 1.0f || outSideVol) break;

            //float lastDis = curDis;
            dst += curRadius * 1.5;/*dst / (1.0 - aperture);*/
        }
        diffuseCol += VC4.rgb * ConeSampleWeights[conei];
        //diffusOcclusion += VA * ConeSampleWeights[conei];
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
        if (gShadowMap.Sample(gsamLinearWrap, curPos).r + 0.002 > shadowPosNDC.z) {
            percentLit += 0.1f;
        }
    }
    //percentLit += 0.1f;
    float lit = 3.0;
    float3 lightDir = gLightPosW;
    lightDir = normalize(lightDir);
    float lamb = dot(lightDir, Nor) + 0.1;

    float4 col = float4(Alb.rgb, 1.0f);
    col = clamp(col * lamb * percentLit, float4(0.0,0.0,0.0,0.0),float4(1.0,1.0,1.0,1.0) ) ;

    col.xyz *= lit;
    col = col + float4(diffuseCol * Alb.xyz * lit, 0.0);
    col.a = 1.0;

    if (showDirect) {
        col = float4(Alb.rgb * lamb * percentLit* lit, 1);
    }
    if (visulizevoxel == 1) {
        col = float4(voxelPickColor, 1.0f);
    }
    else if (visulizevoxel == 2) {
        col = float4(voxelNormal.xyz, 1.0f);
    }

    //col.xyz = diffuseCol * lit;
 

    float gamma = 0.9;
    float exposure = 1.8;

    float3 mapped = float3(1.0,1.0,1.0) - exp(-col.xyz * exposure);

    mapped = pow(mapped, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));

    col = float4(mapped, 1.0);

 
    return col;
}


