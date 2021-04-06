


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
    float4 screenSpacePos = float4(uv * 2.0 - 1.0, gShadowMap.Load(int3(DTid.xy, 0)).x - 0.001 , 1.0);
    screenSpacePos.y = -screenSpacePos.y;

    float4 volumeSpacePos = mul(screenSpacePos, gLight2World);
    volumeSpacePos.xyz /= (voxelScale);

    uint3 texIndex = uint3(((volumeSpacePos.x * 0.5) + 0.5f) * volTexDimensions.x,
        ((volumeSpacePos.y * 0.5) + 0.5f) * volTexDimensions.y + 1, // not sre why, but need to offset y value to match accurate result, weird
        ((volumeSpacePos.z * 0.5) + 0.5f) * volTexDimensions.z );

    float4 col = float4(convRGBA8ToVec4(gVoxelizerAlbedo[texIndex]).xyz / 255.0, 1.0f);

    float3 nor = float3(convRGBA8ToVec4(gVoxelizerNormal[texIndex]).xyz / 255.0);
    nor = 2.0 * (nor  - float3(0.5, 0.5, 0.5));

    col.xyz *= (abs(dot(nor, -gLightDir))) * gLightCol;

    //col.xyz = float3(1.0, 1.0, 1.0);

    col *= 255.0f;

    //imageAtomicRGBA8Avg(gVoxelizerRadiance, texIndex, col);
    gVoxelizerRadiance[int3(texIndex)] = convVec4ToRGBA8(col);
}