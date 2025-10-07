cbuffer constants : register(b0)
{
    row_major float4x4 world;
	uint UUID;
	float3 Pad;
}

cbuffer PerFrame : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    uint ViewModeIndex;
    float3 Padding;
};

cbuffer PerDrawColor : register(b2)
{
    float4 totalColor;
};

struct VS_INPUT
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

struct PS_OUTPUT
{
	float4 FinalColor : SV_Target0;
	uint UUID : SV_Target1;
};

PS_INPUT MainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    float4 Position = Input.Position;
		
    Position = mul(Position, world);
    Position = mul(Position, ViewMatrix);
    Position = mul(Position, ProjectionMatrix);

    Output.Position = Position;
    Output.Color = Input.Color;
    return Output;
}

PS_OUTPUT MainPS(PS_INPUT Input)
{
	PS_OUTPUT Result;
    float4 FinalColor = lerp(Input.Color, totalColor, totalColor.a);
	Result.FinalColor = FinalColor;
	Result.UUID = UUID;
    return Result;
}
