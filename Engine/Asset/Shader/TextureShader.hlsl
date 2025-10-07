cbuffer Model : register(b0)
{
	row_major float4x4 ModelMatrix;
	uint UUID;
	float3 Pad0;
};

cbuffer PerFrame : register(b1)
{
	row_major float4x4 ViewMatrix;
	row_major float4x4 ProjectionMatrix;
	uint ViewModeIndex;
	float3 Pad1;
};

// Billboard용: UVRect.xy = Offset, UVRect.zw = Size
cbuffer BillboardCB : register(b4)
{
	float4 UVRect; // (U, V, UL, VL)
	float4 SpriteColor; // (r,g,b,a)
}

Texture2D SpriteTex : register(t0);
SamplerState SpriteSamp : register(s0);

struct VS_INPUT
{
	float3 Position : POSITION; // CPU가 만들어준 월드 좌표 or 카메라-빌보드 변환 전 좌표
	float2 UV : TEXCOORD0;
};

struct PS_INPUT
{
	float4 HPos : SV_Position;
	float2 UV : TEXCOORD0;
	float4 Color : COLOR;
	uint UUID : UUID;
};

struct PS_OUTPUT
{
	float4 Color : SV_Target0;
	uint UUID : SV_Target1;
};

PS_INPUT mainVS(VS_INPUT In)
{
	PS_INPUT Out;
	float4 wp = mul(float4(In.Position, 1.0), ViewMatrix);
	Out.HPos = mul(wp, ProjectionMatrix);

	Out.UV = UVRect.xy + In.UV * UVRect.zw;
	Out.Color = SpriteColor;
	Out.UUID = UUID;
	return Out;
}

PS_OUTPUT mainPS(PS_INPUT In)
{
	PS_OUTPUT Out;

	if (ViewModeIndex == 2) // Wire
	{
		Out.Color = float4(1, 1, 1, 1);
		Out.UUID = In.UUID;
		return Out;
	}

	float4 tex = SpriteTex.Sample(SpriteSamp, In.UV);
	Out.Color = tex * In.Color;
	Out.UUID = In.UUID;
	return Out;
}
