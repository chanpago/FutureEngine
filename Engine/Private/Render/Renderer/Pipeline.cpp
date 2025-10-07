#include "pch.h"
#include "Render/Renderer/Pipeline.h"
#include "Public/Manager/Resource/ResourceManager.h"
/// @brief 그래픽 파이프라인을 관리하는 클래스
UPipeline::UPipeline(ID3D11DeviceContext* InDeviceContext, ID3D11Device* InDevice)
	: DeviceContext(InDeviceContext), Device(InDevice)
{
	CreateBlendState();
	CreateDepthStencilState();
}

UPipeline::~UPipeline()
{

	for (auto& Pair : DepthStencilStates)
	{
		Pair.second->Release();
	}
	for (auto& Pair : RasterizerStates)
	{
		Pair.second->Release();
	}
	for (auto& Pair : BlendStates)
	{
		Pair.second->Release();
	}
}

//RaterizerState와 PipelineState를 제외하고는 모두 이미 생성된 것을 써야함(enum이 키값).
//
const FPipelineInfo& UPipeline::GetOrCreatePipelineState(const FPipelineDescKey& InKey)
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	if (Pipelines.Find(InKey))
	{
		return Pipelines[InKey];
	}
	FPipelineInfo PipelineInfo = {};
	PipelineInfo.BlendState = BlendStates[InKey.BlendType];
	PipelineInfo.DepthStencilState = DepthStencilStates[InKey.DepthStencilType];
	const FShader& Shader = ResourceManager.GetShader(InKey.ShaderType);
	PipelineInfo.InputLayout = Shader.InputLayout;
	PipelineInfo.VertexShader = Shader.VertexShader;
	PipelineInfo.PixelShader = Shader.PixelShader;
	PipelineInfo.Topology = InKey.Topology;

	PipelineInfo.RasterizerState = GetOrCreateRasterizerState(InKey.RasterizerKey);

	Pipelines.emplace(InKey, PipelineInfo);

	return Pipelines[InKey];

}

/// @brief 파이프라인 상태를 업데이트
void UPipeline::UpdatePipeline(FPipelineInfo Info)
{
	DeviceContext->IASetPrimitiveTopology(Info.Topology);
	if (Info.InputLayout)
		DeviceContext->IASetInputLayout(Info.InputLayout);
	if (Info.VertexShader)
		DeviceContext->VSSetShader(Info.VertexShader, nullptr, 0);
	if (Info.RasterizerState)
		DeviceContext->RSSetState(Info.RasterizerState);
	if (Info.DepthStencilState)
		DeviceContext->OMSetDepthStencilState(Info.DepthStencilState, 0);
	if (Info.PixelShader)
		DeviceContext->PSSetShader(Info.PixelShader, nullptr, 0);
	if (Info.BlendState)
		DeviceContext->OMSetBlendState(Info.BlendState, nullptr, 0xffffffff);
}

/// @brief 정점 버퍼를 바인딩
void UPipeline::SetVertexBuffer(ID3D11Buffer* VertexBuffer, uint32 Stride)
{
	uint32 Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
}

void UPipeline::SetInstanceBuffer(ID3D11Buffer* InstanceBuffer, uint32 Stride)
{
	uint32 Offset = 0;
	DeviceContext->IASetVertexBuffers(1, 1, &InstanceBuffer, &Stride, &Offset);
}

/// @brief 상수 버퍼를 설정
void UPipeline::SetConstantBuffer(uint32 Slot, bool bIsVS, ID3D11Buffer* ConstantBuffer)
{
	if (ConstantBuffer)
	{
		if (bIsVS)
			DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
		else
			DeviceContext->PSSetConstantBuffers(Slot, 1, &ConstantBuffer);
	}
}

/// @brief 셰이더 리소스 뷰 설정
void UPipeline::SetShaderResourceView(uint32 Slot, bool bIsVS, ID3D11ShaderResourceView* ShaderResourceView)
{
	if (bIsVS)
		DeviceContext->VSSetShaderResources(Slot, 1, &ShaderResourceView);
	else
		DeviceContext->PSSetShaderResources(Slot, 1, &ShaderResourceView);
}

/// @brief 샘플러 상태를 설정
void UPipeline::SetSamplerState(uint32 Slot, bool bIsVS, ID3D11SamplerState* SamplerState)
{
	if (bIsVS)
		DeviceContext->VSSetSamplers(Slot, 1, &SamplerState);
	else
		DeviceContext->PSSetSamplers(Slot, 1, &SamplerState);
}

/// @brief 정점 개수를 기반으로 드로우 호출
void UPipeline::Draw(uint32 VertexCount, uint32 StartLocation)
{
	DeviceContext->Draw(VertexCount, StartLocation);
}

void UPipeline::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 VertexStartLocation, uint32 InstanceStartLocation)
{
	DeviceContext->DrawInstanced(VertexCountPerInstance, InstanceCount, VertexStartLocation, InstanceStartLocation);
}

/// @brief 인덱스 버퍼를 설정
void UPipeline::SetIndexBuffer(ID3D11Buffer* IndexBuffer, DXGI_FORMAT Format)
{
	DeviceContext->IASetIndexBuffer(IndexBuffer, Format, 0);
}

/// @brief 인덱스 버퍼를 사용한 드로우 호출
void UPipeline::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, int32 BaseVertexLocation)
{
	DeviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void UPipeline::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int32 BaseVertexLocation, uint32 StartInstanceLocation)
{
	DeviceContext->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}


void UPipeline::CreateDepthStencilState()
{

	D3D11_DEPTH_STENCIL_DESC DescDefault = {};

	DescDefault.DepthEnable = TRUE;
	DescDefault.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	DescDefault.DepthFunc = D3D11_COMPARISON_LESS;

	DescDefault.StencilEnable = FALSE;

	ID3D11DepthStencilState* State = nullptr;
	HRESULT hr = Device->CreateDepthStencilState(
		&DescDefault,
		&State
	);
	DepthStencilStates.emplace(EDepthStencilType::Opaque, State);

	D3D11_DEPTH_STENCIL_DESC descDisabled = {};

	descDisabled.DepthEnable = FALSE;

	descDisabled.StencilEnable = FALSE;


	hr = Device->CreateDepthStencilState(
		&descDisabled,
		&State
	);

	DepthStencilStates.emplace(EDepthStencilType::DepthDisable, State);

	D3D11_DEPTH_STENCIL_DESC DescText = {};

	//Text(외 투명한 물체)는 깊이테스트를 하되 쓰지는 않아야 함.
	DescText.DepthEnable = TRUE;
	DescText.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DescText.DepthFunc = D3D11_COMPARISON_LESS;
	DescText.StencilEnable = FALSE;


	hr = Device->CreateDepthStencilState(
		&DescText,
		&State
	);

	DepthStencilStates.emplace(EDepthStencilType::Transparent, State);
}
void UPipeline::CreateBlendState()
{

    D3D11_BLEND_DESC BlendDesc = {};
    // Enable per-RT blend configuration so RT0 can blend while RT1 (ID buffer) does not
    BlendDesc.IndependentBlendEnable = TRUE;

    D3D11_RENDER_TARGET_BLEND_DESC& Rt0 = BlendDesc.RenderTarget[0];

	Rt0.BlendEnable = TRUE;

	Rt0.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	Rt0.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	Rt0.BlendOp = D3D11_BLEND_OP_ADD;
	
	Rt0.SrcBlendAlpha = D3D11_BLEND_ONE;
	Rt0.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	Rt0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	
	Rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


	D3D11_RENDER_TARGET_BLEND_DESC& Rt1 = BlendDesc.RenderTarget[1];
	Rt1.BlendEnable = FALSE;
	Rt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* State = nullptr;

	Device->CreateBlendState(&BlendDesc, &State);

	BlendStates.emplace(EBlendType::Transparent, State);

	State = nullptr;

    BlendDesc = {};
    BlendDesc.IndependentBlendEnable = TRUE;
    Rt0 = BlendDesc.RenderTarget[0];
	Rt0.BlendEnable = FALSE;
	Rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	Rt1 = BlendDesc.RenderTarget[1];
	Rt1.BlendEnable = FALSE;
	Rt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	Device->CreateBlendState(&BlendDesc, &State);

	BlendStates.emplace(EBlendType::Opaque, State);

  //  D3D11_BLEND_DESC BlendDesc = {};
  //  // Transparent: Enable blending for RT0 (color), but ensure RT1 (ID buffer) writes are enabled with no blending
  //  {
  //      BlendDesc.IndependentBlendEnable = TRUE; // per-RT blend settings
  //      // Initialize all RT slots to disabled but writable by default

  //      BlendDesc.RenderTarget[0].BlendEnable = TRUE;
  //      BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		//BlendDesc.RenderTarget[1].BlendEnable = FALSE;
		//BlendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  //      // RT0: standard alpha blending for color
  //      D3D11_RENDER_TARGET_BLEND_DESC& Rt0 = BlendDesc.RenderTarget[0];
  //      Rt0.BlendEnable = TRUE;
		//Rt0.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		//Rt0.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		//Rt0.BlendOp = D3D11_BLEND_OP_ADD;
		//
		//Rt0.SrcBlendAlpha = D3D11_BLEND_ONE;
		//Rt0.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		//Rt0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		//
		//Rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  //      // RT1: no blending, but allow writes (ID buffer)
  //      D3D11_RENDER_TARGET_BLEND_DESC& Rt1 = BlendDesc.RenderTarget[1];
  //      Rt1.BlendEnable = FALSE;
  //      Rt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  //      ID3D11BlendState* State = nullptr;
  //      Device->CreateBlendState(&BlendDesc, &State);
  //      BlendStates.emplace(EBlendType::Transparent, State);
  //  }

  //  // Opaque: No blending; enable writes on RT0 and RT1
  //  {
  //      ZeroMemory(&BlendDesc, sizeof(BlendDesc));
		//BlendDesc.IndependentBlendEnable = TRUE; // per-RT write masks
		//D3D11_RENDER_TARGET_BLEND_DESC& Rt0 = BlendDesc.RenderTarget[0];
		//Rt0 = BlendDesc.RenderTarget[0];
  //      
		//BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		//Rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		//Rt0.BlendEnable = FALSE;

		//D3D11_RENDER_TARGET_BLEND_DESC& Rt1 = BlendDesc.RenderTarget[1];
		//Rt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		//Rt1.BlendEnable = FALSE;
		//BlendDesc.RenderTarget[1].BlendEnable = FALSE;
		//BlendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  //      ID3D11BlendState* State = nullptr;
  //      Device->CreateBlendState(&BlendDesc, &State);
  //      BlendStates.emplace(EBlendType::Opaque, State);
  //  }
}


ID3D11RasterizerState* UPipeline::GetOrCreateRasterizerState(const FRasterizerKey& InRasterizerKey)
{

	if (auto It = RasterizerStates.find(InRasterizerKey); It != RasterizerStates.end())
		return It->second;

	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	// RasterizerDesc.FrontCounterClockwise = TRUE; // CCW를 앞면으로 보겠다! (기본값은 CW)
	RasterizerDesc.FillMode = InRasterizerKey.FillMode;
	RasterizerDesc.CullMode = InRasterizerKey.CullMode;
	RasterizerDesc.DepthClipEnable = TRUE; // ✅ 근/원거리 평면 클리핑 활성화 (핵심)
	RasterizerDesc.ScissorEnable = TRUE;   // ✅ RSSetScissorRects 사용

	HRESULT Hr = Device->CreateRasterizerState(&RasterizerDesc, &RasterizerState);

	if (FAILED(Hr)) { return nullptr; }

	RasterizerStates.emplace(InRasterizerKey, RasterizerState);
	return RasterizerState;
}
