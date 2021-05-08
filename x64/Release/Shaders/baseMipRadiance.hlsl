
// referenced leifNode's engine : https://github.com/LeifNode/Novus-Engine
RWTexture3D<uint> gVoxelizerRadiance : register(u0);
RWTexture3D<uint> gVoxelizerMipedRadiance : register(u1);

cbuffer CB : register(b0) {
	int mipDim;
}

float4 convRGBA8ToVec4(uint value)
{
	return float4(float((value & 0x000000FF)), float((value & 0x0000FF00) >> 8U), float((value & 0x00FF0000) >> 16U), float((value & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(float4 val)
{
	return (uint (val.w) & 0x000000FF) << 24U | (uint(val.z) & 0x000000FF) << 16U | (uint(val.y) & 0x000000FF) << 8U | (uint(val.x) & 0x000000FF);
}


// billinear filtering
float4 filterAnsiotropicVoxelDirection(float4 f1, float4 f2, float4 f3, float4 f4, float4 b1, float4 b2, float4 b3, float4 b4)
{
	float4 directionalAccum = float4(0.0f, 0.0f, 0.0f, 0.0f);

	directionalAccum = (f1 + b1 * (1.0f - f1.a)) +
		(f2 + b2 * (1.0f - f2.a)) +
		(f3 + b3 * (1.0f - f3.a)) +
		(f4 + b4 * (1.0f - f4.a));

	return directionalAccum * 0.25f;
}

[numthreads(8, 8, 8)]
void BaseMipRadiance( uint3 DTid : SV_DispatchThreadID )
{

	uint3 curMipIdx = DTid;

	if (gVoxelizerRadiance[curMipIdx] == 0 && gVoxelizerMipedRadiance[curMipIdx] == 0) return;
	uint3 moveDim = uint3(mipDim, 0, 0);
	gVoxelizerMipedRadiance[curMipIdx] = gVoxelizerRadiance[curMipIdx];
	gVoxelizerMipedRadiance[curMipIdx + moveDim] = gVoxelizerRadiance[curMipIdx];
	gVoxelizerMipedRadiance[curMipIdx + moveDim *2] = gVoxelizerRadiance[curMipIdx];
	gVoxelizerMipedRadiance[curMipIdx + moveDim *3] = gVoxelizerRadiance[curMipIdx];
	gVoxelizerMipedRadiance[curMipIdx + moveDim *4] = gVoxelizerRadiance[curMipIdx];
	gVoxelizerMipedRadiance[curMipIdx + moveDim *5] = gVoxelizerRadiance[curMipIdx];
	
}

[numthreads(8, 8, 8)]
void AllMipRadiance(uint3 DTid : SV_DispatchThreadID)
{
	int3 biggerMipIdx = DTid * 2;
	uint3 curMipIdx = DTid;

	int biggerMipIncrement = mipDim * 2;
	int curMipIncrement = mipDim;


	float4 filteredCol[8];
	int i, j, k;
	[unroll]
	for (i = 0; i < 8; ++i) {
		filteredCol[i] = float4(0.0, 0.0, 0.0, 0.0);
	}



	//+x
	[unroll]
	for (i = 0; i < 2; i++)
	{
		[unroll]
		for (j = 0; j < 2; j++)
		{
			[unroll]
			for (k = 0; k < 2; k++)
			{
				int3 curIdx = biggerMipIdx + int3(i, j, k);
				filteredCol[4 * i + 2 * j + 1 * k] = convRGBA8ToVec4(gVoxelizerRadiance[curIdx]) / 255.0;
			}
		}
	}
	float4 colPX = filterAnsiotropicVoxelDirection(filteredCol[0], filteredCol[1], filteredCol[2], filteredCol[3], filteredCol[4], filteredCol[5], filteredCol[6], filteredCol[7]);
	gVoxelizerMipedRadiance[curMipIdx] = convVec4ToRGBA8(colPX * 255.0);
	curMipIdx.x += curMipIncrement;
	biggerMipIdx.x += biggerMipIncrement;

	// -x
	[unroll]
	for (i = 0; i < 2; i++)
	{
		[unroll]
		for (j = 0; j < 2; j++)
		{
			[unroll]
			for (k = 0; k < 2; k++)
			{
				int3 curIdx = biggerMipIdx + int3(i, j, k);
				filteredCol[- 4 * i + 2 * j + 1 * k + 4] = convRGBA8ToVec4(gVoxelizerRadiance[curIdx]) / 255.0;
			}
		}
	}
	float4 colNX = filterAnsiotropicVoxelDirection(filteredCol[0], filteredCol[1], filteredCol[2], filteredCol[3], filteredCol[4], filteredCol[5], filteredCol[6], filteredCol[7]);
	gVoxelizerMipedRadiance[curMipIdx] = convVec4ToRGBA8(colNX * 255.0);
	curMipIdx.x += curMipIncrement;
	biggerMipIdx.x += biggerMipIncrement;

	// +y
	[unroll]
	for (i = 0; i < 2; i++)
	{
		[unroll]
		for (j = 0; j < 2; j++)
		{
			[unroll]
			for (k = 0; k < 2; k++)
			{
				int3 curIdx = biggerMipIdx + int3(i, j, k);
				filteredCol[1 * i + 4 * j + 2 * k] = convRGBA8ToVec4(gVoxelizerRadiance[curIdx]) / 255.0;
			}
		}
	}
	float4 colPY = filterAnsiotropicVoxelDirection(filteredCol[0], filteredCol[1], filteredCol[2], filteredCol[3], filteredCol[4], filteredCol[5], filteredCol[6], filteredCol[7]);
	gVoxelizerMipedRadiance[curMipIdx] = convVec4ToRGBA8(colPY * 255.0);
	curMipIdx.x += curMipIncrement;
	biggerMipIdx.x += biggerMipIncrement;

	// -y
	[unroll]
	for (i = 0; i < 2; i++)
	{
		[unroll]
		for (j = 0; j < 2; j++)
		{
			[unroll]
			for (k = 0; k < 2; k++)
			{
				int3 curIdx = biggerMipIdx + int3(i, j, k);
				filteredCol[1 * i - 4 * j + 2 * k + 4] = convRGBA8ToVec4(gVoxelizerRadiance[curIdx]) / 255.0;
			}
		}
	}
	float4 colNY = filterAnsiotropicVoxelDirection(filteredCol[0], filteredCol[1], filteredCol[2], filteredCol[3], filteredCol[4], filteredCol[5], filteredCol[6], filteredCol[7]);
	gVoxelizerMipedRadiance[curMipIdx] = convVec4ToRGBA8(colNY * 255.0);
	curMipIdx.x += curMipIncrement;
	biggerMipIdx.x += biggerMipIncrement;

	// +z
	[unroll]
	for (i = 0; i < 2; i++)
	{
		[unroll]
		for (j = 0; j < 2; j++)
		{
			[unroll]
			for (k = 0; k < 2; k++)
			{
				int3 curIdx = biggerMipIdx + int3(i, j, k);
				filteredCol[1 * i + 2 * j + 4 * k] = convRGBA8ToVec4(gVoxelizerRadiance[curIdx]) / 255.0;
			}
		}
	}
	float4 colPZ = filterAnsiotropicVoxelDirection(filteredCol[0], filteredCol[1], filteredCol[2], filteredCol[3], filteredCol[4], filteredCol[5], filteredCol[6], filteredCol[7]);
	gVoxelizerMipedRadiance[curMipIdx] = convVec4ToRGBA8(colPZ * 255.0);
	curMipIdx.x += curMipIncrement;
	biggerMipIdx.x += biggerMipIncrement;

	// -z
	[unroll]
	for (i = 0; i < 2; i++)
	{
		[unroll]
		for (j = 0; j < 2; j++)
		{
			[unroll]
			for (k = 0; k < 2; k++)
			{
				int3 curIdx = biggerMipIdx + int3(i, j, k);
				filteredCol[1 * i + 2 * j - 4 * k + 4] = convRGBA8ToVec4(gVoxelizerRadiance[curIdx]) / 255.0;
			}
		}
	}
	float4 colNZ = filterAnsiotropicVoxelDirection(filteredCol[0], filteredCol[1], filteredCol[2], filteredCol[3], filteredCol[4], filteredCol[5], filteredCol[6], filteredCol[7]);
	gVoxelizerMipedRadiance[curMipIdx] = convVec4ToRGBA8(colNZ * 255.0);
}