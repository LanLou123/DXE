

cbuffer cbRadiance : register(b0)
{
    float3 gLightDir;
    float pad0;
    float3 gLightCol;
    float pad1;
    float4x4 gLight2World;
    float voxelScale;
};



RWTexture3D<uint> gVoxelizerAlbedo : register(u0);
RWTexture3D<uint> gVoxelizerNormal : register(u1);
RWTexture3D<uint> gVoxelizerEmissive : register(u2);
RWTexture3D<uint> gVoxelizerRadiance : register(u3);

Texture2D    gShadowMap : register(t0);

[numthreads(16, 16, 1)]
void Radiance( uint3 DTid : SV_DispatchThreadID )
{

}