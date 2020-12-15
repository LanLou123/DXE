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
    return col * lamb * percentLit;
}


