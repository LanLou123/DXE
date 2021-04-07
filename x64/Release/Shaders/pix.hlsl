#include "common.hlsl"
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
    float3 voxelNormal = float3(0.0, 0.0, 0.0);
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
            uint3 texIndex = uint3(((mappedloc.x * 0.5) + 0.5f) * texDimensions.x,
                ((mappedloc.y * 0.5) + 0.5f) * texDimensions.y,
                ((mappedloc.z * 0.5) + 0.5f) * texDimensions.z);
            if (visulizevoxel == 2) {
                if (gVoxelizerRadiance[texIndex] == 0) {
                    curloc = curloc + raydr * step;
                }
                else {

                    voxelPickColor = convRGBA8ToVec4(gVoxelizerAlbedo[texIndex]).xyz / 255.0;
                    voxelNormal = convRGBA8ToVec4(gVoxelizerRadiance[texIndex]).xyz / 255.0;
                    step = -0.1 * step;
                    curloc = curloc + raydr * step;

                }
            }
            else {
                if (gVoxelizerAlbedo[texIndex] == 0) {
                    curloc = curloc + raydr * step;
                }

                else {

                    voxelPickColor = convRGBA8ToVec4(gVoxelizerAlbedo[texIndex]).xyz / 255.0;
                    voxelNormal = convRGBA8ToVec4(gVoxelizerRadiance[texIndex]).xyz / 255.0;
                    step = -0.1 * step;
                    curloc = curloc + raydr * step;

                }
            }



        }
    }

    // =======================================
    // screen space shadow mapping
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
    percentLit += 0.1f;

    float3 lightDir = gLightPosW;
    lightDir = normalize(lightDir);
    float lamb = dot(lightDir, Nor);

    float4 col = float4(gAlbedoMap.Sample(gsamLinearWrap, pin.Texc).rgb, 1.0f);
    col = col * lamb * percentLit;
    if (visulizevoxel == 1) {
        col = float4(voxelPickColor, 1.0f);
    }
    else if (visulizevoxel == 2) {
        col = float4(voxelNormal, 1.0f);
    }
    return col;
}


