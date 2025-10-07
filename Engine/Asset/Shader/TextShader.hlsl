cbuffer Model : register(b0)
{
	row_major float4x4 ModelMatrix;
	uint UUID;
	float3 Pad;
}

cbuffer PerFrame : register(b1)
{
	row_major float4x4 ViewMatrix; // View Matrix Calculation of MVP Matrix
	row_major float4x4 ProjectionMatrix; // Projection Matrix Calculation of MVP Matrix
	uint ViewModeIndex; // View Mode (0: Lit, 1: Unlit, 2: WireFrame)
	float3 Padding;
};

struct CharUv
{
	float2 UvOffset;
	float2 UvSize;
};

cbuffer CharTable : register(b4)
{
	CharUv UvTable[2446];
}

Texture2D FontAtlas : register(t0);

SamplerState Sampler : register(s0);


struct VS_INPUT
{
	float3 Position : POSITION;
	float2 UV : TEXCOORD0;
	uint CharID : TEXCOORD1;
};

struct PS_INPUT
{
	float4 WorldPos : SV_Position;
	float4 Color : COLOR;
	float2 UV : TEXCOORD0;
	uint UUID : UUID;
};

struct PS_OUTPUT
{
	float4 FinalColor : SV_Target0;
	uint UUID : SV_Target1;
};

float3 GetCameraForward();

PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output;

	float FontScale = 1 / 3.0f;
	float3 BasePos = float3(Input.Position.x, Input.Position.y, Input.Position.z) * FontScale;

	float4 world= mul(float4(BasePos, 1), ModelMatrix);
	float4 view = mul(world, ViewMatrix);
	float4 clip = mul(view, ProjectionMatrix);

	Output.WorldPos = clip;
	Output.Color = float4(1, 1, 1, 1);
	Output.UV = UvTable[Input.CharID].UvSize * Input.UV + UvTable[Input.CharID].UvOffset;
	Output.UUID = UUID;

	return Output;
}

PS_OUTPUT mainPS(PS_INPUT Input)
{
	PS_OUTPUT Result;
	if (ViewModeIndex == 2)
	{
		Result.FinalColor = float4(1, 1, 1, 1);
		Result.UUID = Input.UUID;
		return Result;
	}
	float4 TextureColor = FontAtlas.Sample(Sampler, Input.UV);
	Result.FinalColor = TextureColor;
	Result.UUID = Input.UUID;
	return Result;
}

float3 GetCameraForward()
{
	float3 Result;

	float3x3 RotationMatrixInverse = transpose(float3x3(ViewMatrix[0].xyz, ViewMatrix[1].xyz, ViewMatrix[2].xyz));

	
	return RotationMatrixInverse[2].xyz;
}
