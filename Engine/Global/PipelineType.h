#pragma once


struct FPipelineInfo
{
	ID3D11InputLayout* InputLayout;
	ID3D11VertexShader* VertexShader;
	ID3D11RasterizerState* RasterizerState;
	ID3D11DepthStencilState* DepthStencilState;
	ID3D11PixelShader* PixelShader;
	ID3D11BlendState* BlendState;
	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};
struct FShader
{
public:
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
	ID3D11InputLayout* InputLayout;
};

enum class EShaderType
{
	SampleShader,
	StaticMeshShader,
	TextShader,
	LineInstanceShader,
	TextureShader,
};

//sturct Key를 쓰기엔 아직 Stencil을 안 쓰고 DepthTest를 끄고 DepthFunction을 설정하는 등의 에러타입이 많아서 일단 enum을 씀
enum class EDepthStencilType
{
	Opaque,	//depth on stencil disalble
	Transparent,
	DepthDisable,
};

//블랜드는 조합이 단순해서 그냥 enum으로 처리
enum class EBlendType
{
	Opaque,
	Transparent,
};

//Rasterizer는 Fill Cull모드가 독립적이라 struct 키를 씀.
struct FRasterizerKey
{
	D3D11_FILL_MODE FillMode = {};
	D3D11_CULL_MODE CullMode = {};

	bool operator==(const FRasterizerKey& InKey) const
	{
		return FillMode == InKey.FillMode && CullMode == InKey.CullMode;
	}
};

struct FRasterizerKeyHasher
{
	size_t operator()(const FRasterizerKey& InKey) const noexcept
	{
		auto Mix = [](size_t& H, size_t V)
			{
				H ^= V + 0x9e3779b97f4a7c15ULL + (H << 6) + (H << 2);
			};

		size_t H = 0;
		Mix(H, (size_t)InKey.FillMode);
		Mix(H, (size_t)InKey.CullMode);

		return H;
	}
};

struct FPipelineDescKey
{
	EShaderType ShaderType;
	FRasterizerKey RasterizerKey;
	EDepthStencilType DepthStencilType;
	EBlendType BlendType;
	D3D11_PRIMITIVE_TOPOLOGY Topology;

	bool operator==(const FPipelineDescKey& OtherDesc) const
	{
		return ShaderType == OtherDesc.ShaderType &&
			RasterizerKey == OtherDesc.RasterizerKey && // FRasterizerKey의 operator== 호출
			DepthStencilType == OtherDesc.DepthStencilType &&
			BlendType == OtherDesc.BlendType &&
			Topology == OtherDesc.Topology;
	}
};

struct FPipelineDescHasher
{
	size_t operator()(const FPipelineDescKey& Key) const
	{
		auto Mix = [](size_t& H, size_t V)
			{
				H ^= V + 0x9e3779b97f4a7c15ULL + (H << 6) + (H << 2);
			};
		size_t Hash = 0;

		Mix(Hash, static_cast<size_t>(Key.ShaderType));
		Mix(Hash, static_cast<size_t>(Key.DepthStencilType));
		Mix(Hash, static_cast<size_t>(Key.BlendType));
		Mix(Hash, static_cast<size_t>(Key.Topology));

		FRasterizerKeyHasher RasterizerKeyHasher;
		Mix(Hash, RasterizerKeyHasher(Key.RasterizerKey));

		return Hash;

	}
};
