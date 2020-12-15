#include "common.hlsl"

struct VertexIn
{
    float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};
struct VertexOut
{
    float4 PosH  : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout= (VertexOut)0.0f;;

    // Already in homogeneous clip space.
    vout.PosH = float4((vin.PosL / 2.0f) + float3(0.50f, -0.50f, 0.0f), 1.0f);

    // Just pass vertex color into the pixel shader.
    vout.TexC = vin.TexC;
    vout.Normal = vin.Normal;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
 
    return float4(gShadowMap.Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);
}



