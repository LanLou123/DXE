

RWTexture3D<uint> gVoxelizerRadiance : register(u0);
RWTexture3D<uint> gVoxelizerMipedRadiance : register(u1);

cbuffer CB : register(b0) {
	int mipDim;
}

[numthreads(8, 8, 8)]
void BaseMipRadiance( uint3 DTid : SV_DispatchThreadID )
{
	gVoxelizerMipedRadiance[DTid] = mipDim;
}