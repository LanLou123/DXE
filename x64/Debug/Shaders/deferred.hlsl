#include "common.hlsl"

Texture2D    gDiffuseMap : register(t0);
Texture2D    gShadowMap : register(t1);
Texture2D    gPositionMap : register(t2);
Texture2D    gAlbedoMap : register(t3);
Texture2D    gNormalMap : register(t4);
Texture2D    gDepthMap : register(t5);

struct VertexIn
{
    float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};
struct VertexOut
{
    float4 PosW : POSITION;
    float4 PosH  : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};

float linearDepth(float depthSample, float u_near, float u_far)
{
    depthSample = 2.0 * depthSample - 1.0;
    float zLinear = 2.0 * u_near * u_far / (u_far + u_near - depthSample * (u_far - u_near));
    return zLinear;
}

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;


    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
    vout.Normal = normalize(mul(vin.Normal, (float3x3)gWorld));
    vout.TexC = vin.TexC;
    vout.PosW = posW;

    return vout;
}

struct PS_OUT {
    float4 POS: SV_Target0;
    float4 ALB: SV_Target1;
    float4 NOR: SV_Target2;
    float4 DEP: SV_Target3;
};
// This is only used for alpha cut out geometry, so that shadows 
// show up correctly.  Geometry that does not need to sample a
// texture can use a NULL pixel shader for depth pass.
PS_OUT PS(VertexOut pin)
{

    PS_OUT output;

    float4 diffuseAlbedo = 1.0f;

    // Dynamically look up the texture in the array.
    diffuseAlbedo *= gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
    if(IsEmissive == 0)
        output.ALB = diffuseAlbedo;
    else
        output.ALB = 6.0 * diffuseAlbedo;//for emissive material
    output.POS = pin.PosW;
    output.NOR = float4(pin.Normal,1.0f);
    float dpcol = pin.PosH.z;
    output.DEP = float4(dpcol, gRoughness, dpcol, 1.0);
    return output;

}


