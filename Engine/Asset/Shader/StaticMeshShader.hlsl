cbuffer constants : register(b0)
{
	row_major float4x4 World;
	uint UUID;
	float3 Pad;
};

cbuffer PerFrame : register(b1)
{
	row_major float4x4 ViewMatrix;
	row_major float4x4 ProjectionMatrix;
	uint ViewModeIndex;
	float3 Padding;
};

cbuffer PerDrawColor : register(b2)
{
	float4 KdColor;
};

struct VS_INPUT
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float2 BaseUV : TEXTURE;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float2 UV : TEXTURE;
	nointerpolation uint UUID : UUID;
};

struct PS_OUTPUT
{
	float4 FinalColor : SV_Target0;
	uint UUID : SV_Target1;
};

Texture2D Texture : register(t1);
SamplerState Sampler : register(s0);

PS_INPUT MainVS(VS_INPUT Input, uint InstanceId : SV_InstanceID)
{
	PS_INPUT Output;

	float4 Position = Input.Position;
	float4 ShadeColor = Input.Color;



	
	Position = mul(Position, World);
		
	Position = mul(Position, ViewMatrix);
	Position = mul(Position, ProjectionMatrix);

	Output.Position = Position;
	Output.Color = ShadeColor;
	Output.UV = Input.BaseUV;
	Output.UUID = UUID;
	return Output;
}

PS_OUTPUT MainPS(PS_INPUT Input)
{
	PS_OUTPUT Result;
	// 텍스처 적용
	float4 TextureColor = Texture.Sample(Sampler, Input.UV);	

	Result.FinalColor = KdColor * TextureColor;
	Result.UUID = Input.UUID;
	return Result;
}
