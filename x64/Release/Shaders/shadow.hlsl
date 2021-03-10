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

// This is only used for alpha cut out geometry, so that shadows 
// show up correctly.  Geometry that does not need to sample a
// texture can use a NULL pixel shader for depth pass.
void PS(VertexOut pin)
{

    //float4 diffuseAlbedo = 1.0f;

    //// Dynamically look up the texture in the array.
    //diffuseAlbedo *= gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);


}


VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;


    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
    vout.Normal = vin.Normal;
    vout.TexC = vin.TexC;

    return vout;
}