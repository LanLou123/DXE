#include "common.hlsl"
struct VertexIn
{
    float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};

struct GS_INPUT
{
    float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};

struct PS_INPUT {
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};

GS_INPUT VS(VertexIn vin)
{
    GS_INPUT gin;

    gin.PosL = vin.PosL.xyz / 200.0f;

    // Just pass vertex color into the pixel shader.
    gin.TexC = vin.TexC;
    gin.Normal = normalize(mul(vin.Normal, (float3x3)gWorld));
 
    return gin;
}

[maxvertexcount(3)]
void GS(triangle GS_INPUT input[3], inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	float4 outputPosH[3] = { float4(0.0f, 0.0f, 0.0f, 0.0f),
							 float4(0.0f, 0.0f, 0.0f, 0.0f),
							 float4(0.0f, 0.0f, 0.0f, 0.0f) };

	const float2 halfPixel = float2(1.0f, 1.0f) / 512.0f;

	float3 posW0 = mul(float4(input[0].PosL, 1.0f), gWorld).xyz;
	float3 posW1 = mul(float4(input[1].PosL, 1.0f), gWorld).xyz;
	float3 posW2 = mul(float4(input[2].PosL, 1.0f), gWorld).xyz;
	float3 normal = cross(normalize(posW1 - posW0), normalize(posW2 - posW0)); //Get face normal

	float axis[] = {
		abs(dot(normal, float3(1.0f, 0.0f, 0.0f))),
		abs(dot(normal, float3(0.0f, 1.0f, 0.0f))),
		abs(dot(normal, float3(0.0f, 0.0f, 1.0f))),
	};


	//Find dominant axis
	int index = 0;
	int i = 0;

	[unroll]
	for (i = 1; i < 3; i++)
	{
		[flatten]
		if (axis[i] > axis[i - 1])
			index = i;
	}

	[unroll]
	for (i = 0; i < 3; i++)
	{
		float4 inputPosL;

		[flatten]
		switch (index)
		{
		case 0:
			inputPosL = float4(input[i].PosL.yzx, 1.0f);
			break;
		case 1:
			inputPosL = float4(input[i].PosL.zxy, 1.0f);
			break;
		case 2:
			inputPosL = float4(input[i].PosL.xyz, 1.0f);
			break;
		}

		outputPosH[i] = mul(inputPosL, gViewProj);

		output.TexC = input[i].TexC;
		output.PosW = mul( float4(input[i].PosL, 1.0f), gWorld).xyz;
		output.PosH = outputPosH[i];
		output.Normal = input[i].Normal;

		triStream.Append(output);
	}
}

void PS(PS_INPUT pin)
{


	pin.Normal = normalize(pin.Normal);
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

	int3 texDimensions;
	gVoxelizer.GetDimensions(texDimensions.x, texDimensions.y, texDimensions.z);

	uint3 texIndex = uint3(((pin.PosW.x * 0.5) + 0.5f) * texDimensions.x,
		((pin.PosW.y * 0.5) + 0.5f) * texDimensions.y,
		((pin.PosW.z * 0.5) + 0.5f) * texDimensions.z);

	if (all(texIndex < texDimensions.xyz) && all(texIndex >= 0))
	{
		gVoxelizer[texIndex] = diffuseAlbedo;
	}

     
}


