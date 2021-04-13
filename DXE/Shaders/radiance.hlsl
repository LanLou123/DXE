


cbuffer cbRadiance : register(b0)
{
    float3 gLightDir;
    float pad0;
    float3 gLightCol;
    float pad1;
    float4x4 gLight2World;
    float voxelScale;
};

float4 convRGBA8ToVec4(uint val)
{
    float4 re = float4(float((val & 0x000000FF)), float((val & 0x0000FF00) >> 8U), float((val & 0x00FF0000) >> 16U), float((val & 0xFF000000) >> 24U));
    return clamp(re, float4(0.0, 0.0, 0.0, 0.0), float4(255.0, 255.0, 255.0, 255.0));
}

uint convVec4ToRGBA8(float4 val)
{
    return (uint (val.w) & 0x000000FF) << 24U | (uint(val.z) & 0x000000FF) << 16U | (uint(val.y) & 0x000000FF) << 8U | (uint(val.x) & 0x000000FF);
}



RWTexture3D<uint> gVoxelizerAlbedo : register(u0);
RWTexture3D<uint> gVoxelizerNormal : register(u1);
RWTexture3D<uint> gVoxelizerEmissive : register(u2);
RWTexture3D<uint> gVoxelizerRadiance : register(u3);

Texture2D    gShadowMap : register(t0);


float4 getAvgCol(uint3 texIndex) {
    float4 c = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex]).xyz / 255.0, 1.0f);
    float4 t = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex + uint3(0,1,0)]).xyz / 255.0, 1.0f);
    float4 b = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex + uint3(0, -1, 0)]).xyz / 255.0, 1.0f);
    float4 l = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex + uint3(-1, 0, 0)]).xyz / 255.0, 1.0f);
    float4 r = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex + uint3(1, 0, 0)]).xyz / 255.0, 1.0f);
    float4 o = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex + uint3(0, 0, 1)]).xyz / 255.0, 1.0f);
    float4 i = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex + uint3(0, 0, -1)]).xyz / 255.0, 1.0f);
    return (c + t + b + l + r + o + i) / 7;
}

float3 getAvgNorcol(uint3 idx) {

}

[numthreads(16, 16, 1)]
void Radiance( uint3 DTid : SV_DispatchThreadID )
{
    int2 shadowTexDimensions;
    gShadowMap.GetDimensions(shadowTexDimensions.x, shadowTexDimensions.y);

    int3 volTexDimensions;
    gVoxelizerAlbedo.GetDimensions(volTexDimensions.x, volTexDimensions.y, volTexDimensions.z);


    if (DTid.x >= (uint)shadowTexDimensions.x || DTid.y >= (uint)shadowTexDimensions.y)
        return;

    float2 uv = DTid.xy / float2(shadowTexDimensions);
    float4 screenSpacePos = float4(uv * 2.0 - 1.0, gShadowMap.Load(int3(DTid.xy, 0)).x - 0.0063 , 1.0);
    screenSpacePos.y = -screenSpacePos.y;
 

    float4 volumeSpacePos = mul(screenSpacePos, gLight2World);
    volumeSpacePos.xyz /= (voxelScale);

    uint3 texIndex = uint3(((volumeSpacePos.x * 0.5) + 0.5f) * volTexDimensions.x  ,
        ((volumeSpacePos.y * 0.5) + 0.5f) * volTexDimensions.y  , // not sre why, but need to offset y value to match accurate result, weird
        ((volumeSpacePos.z * 0.5) + 0.5f) * volTexDimensions.z );

    float4 col = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex]).xyz / 255.0, 1.0f);
    //float4 col = getAvgCol(texIndex);

    float3 nor = float3(convRGBA8ToVec4(gVoxelizerNormal[texIndex]).xyz / 255.0);
    nor = 2.0 * (nor  - float3(0.5, 0.5, 0.5));

    col.xyz *= (abs(dot(nor, -normalize(gLightDir))) + 0.1) * gLightCol * 1.0;

    //col.xyz = float3(1.0, 1.0, 1.0);



    //imageAtomicRGBA8Avg(gVoxelizerRadiance, texIndex, col);

    int oldRadiance = gVoxelizerRadiance[int3(texIndex)];
    float4 oldRadianceCol = convRGBA8ToVec4(oldRadiance) / 255.0;
    col.a = oldRadianceCol.a; // preserve the alpha
    col *= 255.0f;

    gVoxelizerRadiance[int3(texIndex)] = convVec4ToRGBA8(col);
}