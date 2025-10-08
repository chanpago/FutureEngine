#include "pch.h"
#include "Render/Renderer/Renderer.h"

#include "Level/Level.h"

#include "Manager/Level/World.h"
#include "Manager/UI/UIManager.h"
#include "Actor/Actor.h"
#include "Render/Renderer/Pipeline.h"
#include "Render/Renderer/LineBatchRenderer.h"
#include "Editor/Editor.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "Mesh/Material.h"
#include "Render/UI/Layout/MultiViewBuilders.h"
#include "Render/UI/Layout/Splitter.h"
#include "Manager/Path/PathManager.h"
#include "Manager/Input/InputManager.h"
#include "ImGui/imgui.h"
#include "Math/Octree.h"
#include "Core/ObjectIterator.h"
#include <algorithm>
#include <cstring>
#include "Core/Object.h"
#include "Global/Enum.h"
#include "Math/Math.h"
#include "Manager/Overlay/OverlayManager.h"
#include "Manager/Viewport/ViewportManager.h"
#include "Editor/EditorEngine.h"
#include "public/Render/Viewport/ViewportClient.h"
#include "Render/Viewport/Viewport.h"


namespace
{
	struct FInstanceGPUData
	{
		FMatrix World;
		FVector4 Color;
	};
}

IMPLEMENT_CLASS(URenderer, UObject)
IMPLEMENT_SINGLETON(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext(), GetDevice());

	CreateConstantBuffer();

	/** LineBatchRenderer 초기화 */
	ULineBatchRenderer::GetInstance().Init();

	// Start in single-view layout; multiview is built on demand (F2 toggle)
	MultiViewRoot = nullptr;
	CurrentLayout = EViewportLayout::Single;

	// Create per-viewport cameras
	for (int i = 0; i < 4; ++i)
	{
		ViewCameras[i] = new UCamera();
	}
	ViewCameras[0]->SetCameraType(ECameraType::ECT_Perspective);
	ViewCameras[1]->SetCameraType(ECameraType::ECT_Orthographic); // Top
	ViewCameras[2]->SetCameraType(ECameraType::ECT_Orthographic); // Right
	ViewCameras[3]->SetCameraType(ECameraType::ECT_Orthographic); // Front
	// 오쏘 월드 단위 폭(씬 스케일에 따라 조정)
	// Default ortho widths: make bottom views closer by default
	// Tighter default ortho widths (closer zoom)
	ViewCameras[1]->SetOrthoWorldWidth(30.f);  // Top-Right (Top view)
	ViewCameras[1]->SetLocation(FVector(0, 30, 0));
	ViewCameras[1]->SetRotation(FVector(0, -90, 0));
	ViewCameras[1]->SetNearZ(0.1f); ViewCameras[1]->SetFarZ(1000.f);
	ViewCameras[2]->SetOrthoWorldWidth(30.f);  // Bottom-Left (Right view)
	ViewCameras[2]->SetLocation(FVector(0, 0, 30));
	ViewCameras[2]->SetRotation(FVector(90, 0, 0));
	ViewCameras[2]->SetNearZ(0.1f); ViewCameras[2]->SetFarZ(1000.f);
	ViewCameras[3]->SetOrthoWorldWidth(30.f);
	ViewCameras[3]->SetLocation(FVector(-30, 0, 0));
	ViewCameras[3]->SetRotation(FVector(0, 0, 0));
	ViewCameras[3]->SetNearZ(0.1f); ViewCameras[3]->SetFarZ(1000.f);

	// 강제 단일뷰로 시작 보장
	//SetViewportLayout(EViewportLayout::Single);

	// Load layout from editor.ini
	LoadViewportLayout();

	// Load multi-view camera settings from editor.ini
	LoadMultiViewCameraSettings();

	//MSOC
	DXGI_SWAP_CHAIN_DESC scd = {};
	GetSwapChain()->GetDesc(&scd);
	const int W = (int)scd.BufferDesc.Width;
	const int H = (int)scd.BufferDesc.Height;
	MSOC.Init(W, H);

}

void URenderer::Release()
{
	/** LineBatchRenderer 해제 */
	ULineBatchRenderer::GetInstance().Release();

	// Save current layout to editor.ini
	SaveViewportLayout();

	// Save multi-view camera settings to editor.ini
	SaveMultiViewCameraSettings();

	ReleaseConstantBuffer();

	ReleaseResource();

	ReleaseVertexBuffer(BillboardVertexBuffer);
	BillboardVertexBuffer = nullptr;
	BillboardVertexBufferVertexCount = 0;
	BillboardVertexBufferStride = 0;

	if (MultiViewRoot)
	{
		delete MultiViewRoot;
		MultiViewRoot = nullptr;
	}

	for (int i = 0; i < 4; ++i)
	{
		SafeDelete(ViewCameras[i]);
	}

	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);
}


/**
 * @brief 렌더러에 사용된 모든 리소스를 해제하는 함수
 */
void URenderer::ReleaseResource()
{

	/** 렌더 타겟을 초기화 */
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}





void URenderer::Update(UEditor* Editor)
{
	RenderBegin();

	for (FViewport* Viewport : UViewportManager::GetInstance().GetViewports())
	{
		if (Viewport->GetRect().W < 1.0f || Viewport->GetRect().H < 1.0f)
		{
			continue;
		}

		FRect SingleWindowRect = Viewport->GetRect();
		const int32 ViewportToolBarHeight = 32;
		D3D11_VIEWPORT LocalViewport = { SingleWindowRect.X,SingleWindowRect.Y + ViewportToolBarHeight, SingleWindowRect.W, SingleWindowRect.H - ViewportToolBarHeight, 0.0f, 1.0f };
		GetDeviceContext()->RSSetViewports(1, &LocalViewport);

		if (GWorld->GetWorldType() == EWorldType::PIE)
		{

			UWorld* PIEWorld = GEditor->GetPIEWorldContext().World();
			if (PIEWorld)
			{
				GWorld = PIEWorld;
			}
			else
			{
				GWorld = GEditor->GetEditorWorldContext().World();
			}
		}

		DXGI_SWAP_CHAIN_DESC scd = {};
		GetSwapChain()->GetDesc(&scd);
		LONG maxW = (LONG)scd.BufferDesc.Width;
		LONG maxH = (LONG)scd.BufferDesc.Height;
		LONG left = (LONG)LocalViewport.TopLeftX;
		LONG top = (LONG)LocalViewport.TopLeftY;
		LONG right = (LONG)(LocalViewport.TopLeftX + LocalViewport.Width);
		LONG bottom = (LONG)(LocalViewport.TopLeftY + LocalViewport.Height);
		left = std::max<LONG>(0, std::min<LONG>(left, maxW - 1));
		top = std::max<LONG>(0, std::min<LONG>(top, maxH - 1));
		right = std::max<LONG>(left + 1, std::min<LONG>(right, maxW));
		bottom = std::max<LONG>(top + 1, std::min<LONG>(bottom, maxH));
		D3D11_RECT r{ left, top, right, bottom };
		GetDeviceContext()->RSSetScissorRects(1, &r);


		// 카메라 세팅을 세이브합니다.
		CheckAndSaveCameraSettings();

		// 카메라 세팅
		Cam = Viewport->GetViewportClient()->GetCamera();
		
		// ★★★ 수정된 핵심 로직 ★★★
		// 현재 뷰포트에 맞는 카메라의 데이터를 가져와 Constant Buffer를 직접 업데이트합니다.
		if (Cam)
		{
			UpdateConstant(Cam->GetFViewProjConstants());
		}


		// =================================================================
		// Pass 1 로직을 추가합니다.
		// =================================================================
		RenderLevel();

		// =================================================================
		// 여기에 데칼 렌더링(Pass 2) 로직을 추가합니다.
		// =================================================================
		// RenderDecals();  <-- 이런 함수를 만들어서 호출


		// Transparent billboards (sprites), sort back-to-front per viewport
		RenderBillboards(Cam ? Cam->GetLocation() : Editor->GetCameraLocation());

		//Batch Line Rendering
		Editor->RenderEditorBatched(Cam ? Cam->GetLocation() : Editor->GetCameraLocation());


		// For text sorting, use per-viewport camera location if available
		RenderText(Cam ? Cam->GetLocation() : Editor->GetCameraLocation());
	}
	

	if (CacheWorld)
	{
		GWorld = CacheWorld;
		CacheWorld = nullptr;
	}

	UUIManager::GetInstance().Render();

	RenderEnd();
	static float RefreshTime = 0.0f;
	RefreshTime += DT;
	if (RefreshTime > DT * 20)
	{
		ReadbackIdBuffer();
		RefreshTime = 0;
	}
}

/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin()
{
	ID3D11RenderTargetView* RenderTargetView = DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* IdBufferRTV = DeviceResources->GetIdBufferRTV();
	ID3D11DepthStencilView* DepthStencilView = DeviceResources->GetDepthStencilView();

	float ClearId[4]{ 0, 0, 0, 0 };
	GetDeviceContext()->ClearRenderTargetView(IdBufferRTV, ClearId);
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);


	GetDeviceContext()->RSSetViewports(1, &DeviceResources->GetViewportInfo());

	ID3D11RenderTargetView* RenderTargetViews[] = { RenderTargetView, IdBufferRTV }; // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(2, RenderTargetViews, DeviceResources->GetDepthStencilView());
	DeviceResources->UpdateViewport();
}

void URenderer::RenderLevel()
{

	// 여기에 카메라 VP 업데이트 한 번 싹
	//
	if (!GWorld->GetCurrentLevel()) { return; }

	// Check show flags for primitive components
	if (IsShowFlagEnabled(EEngineShowFlags::SF_Primitives) == false) { return; }

	FPipelineDescKey PipelineDescKey;
	PipelineDescKey.BlendType = EBlendType::Opaque;
	PipelineDescKey.DepthStencilType = EDepthStencilType::Opaque;
	PipelineDescKey.ShaderType = EShaderType::StaticMeshShader;
	PipelineDescKey.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRasterizerKey RasterizerKey;
	RasterizerKey.CullMode = D3D11_CULL_BACK;
	RasterizerKey.FillMode = D3D11_FILL_SOLID;

	if (CurrentViewMode == EViewModeIndex::Wireframe)
	{
		RasterizerKey.CullMode = D3D11_CULL_NONE;
		RasterizerKey.FillMode = D3D11_FILL_WIREFRAME;
	}
	PipelineDescKey.RasterizerKey = RasterizerKey;
	Pipeline->UpdatePipeline(Pipeline->GetOrCreatePipelineState(PipelineDescKey));


	// =================================================================
	// 옥트리를 이용한 프러스텀 컬링 부분
	// =================================================================
	//TOctree<UPrimitiveComponent, PrimitiveComponentTrait>& StaticOctree = GWorld->GetCurrentLevel()->GetStaticOctree();
	//
	//PrimitiveComponentsToRender.clear();
	//
	//StaticOctree.OctreeFrustumCulling(Cam,PrimitiveComponentsToRender);
	//
	//PrimitiveComponentsToRender.Append(StaticOctree.GetElementsOutsideOctree());
	// 
	//int count = 0;
	//
	//UE_LOG("%d", PrimitiveComponentsToRender.Num());

	TArray<UStaticMeshComponent*>& StaticMeshComponentsToRender = GWorld->GetCurrentLevel()->GetStaticMeshComponentsToRender();

	// 컬링된 결과(PrimitiveComponentsToRender)를 순회하여 렌더링
	for (auto& Component : StaticMeshComponentsToRender)
	{

		// UPrimitiveComponent를 UStaticMeshComponent로 캐스팅
		UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component);
		// 캐스팅이 실패하면(nullptr이면) 이 컴포넌트는 StaticMesh가 아니므로 건너뜀 (크래시 방지)
		if (!StaticMeshComponent)
		{
			continue;
		}

		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		if (!StaticMesh)
		{
			continue;
		}
		Pipeline->SetVertexBuffer(StaticMesh->GetVertexBuffer(), StrideStaticMesh);
		Pipeline->SetIndexBuffer(StaticMesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT);


		////////////// 텍스쳐 적용 테스트 코드////////////////
		UResourceManager& ResourceManager = UResourceManager::GetInstance();
		FStaticMesh* Asset = StaticMesh->GetStaticMeshAsset();

		if (Asset && !Asset->Sections.empty())
		{
			for (int Index = 0; Index < Asset->Sections.Num(); Index++)
			{
				ID3D11ShaderResourceView* TextureSRV = nullptr;

				Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
				// 안전하게 캐스팅된 StaticMeshComponent 변수 사용
				UpdateConstant(FModelConstant{ Component->GetWorldTransformMatrix(), Component->GetInternalIndex() });

				// 안전하게 캐스팅된 StaticMeshComponent 변수 사용
				const UMaterial* Material = Component->GetMaterial(Index);
				if (Material)
				{
					if (!Material->GetKdTextureFilePath().empty())
					{
						TextureSRV = ResourceManager.GetTexture(Material->GetKdTextureFilePath());
						if (TextureSRV)
						{
							Pipeline->SetShaderResourceView(1, false, TextureSRV);
							Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
							UpdateConstant(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
						}
					}
					else
					{
						Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
						UpdateConstant(FVector4(Material->GetMaterialInfo().Kd, 1.0f));
					}
				}

				if (!TextureSRV)
				{
					//매터리얼에 텍스쳐가 없거나 텍스처를 로드하지 못한 경우 흰색 텍스쳐 써서 기본 vertex 컬러 출력되도록 함.
					TextureSRV = ResourceManager.GetTexture("Data/None.dds");
					if (TextureSRV)
					{
						Pipeline->SetShaderResourceView(1, false, TextureSRV);
					}
				}
				Pipeline->DrawIndexed(Asset->Sections[Index].IndexCount, Asset->Sections[Index].IndexStart, 0);
			}
		}
	}
	//UE_LOG("OC: %d", count);
}

void URenderer::RenderBillboards(const FVector& CameraLocation)
{
    const TArray<UBillboardComponent*>& List = GWorld->GetCurrentLevel()->GetBillboardComponentsToRender();
    if (List.Num() == 0) return;

    // Pipeline setup
    FPipelineDescKey PipelineDescKey;
    PipelineDescKey.BlendType = EBlendType::Transparent;
    PipelineDescKey.DepthStencilType = EDepthStencilType::Transparent;
    PipelineDescKey.ShaderType = EShaderType::TextureShader;
    PipelineDescKey.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    FRasterizerKey RasterizerKey; RasterizerKey.CullMode = D3D11_CULL_NONE; RasterizerKey.FillMode = D3D11_FILL_SOLID;
    if (CurrentViewMode == EViewModeIndex::Wireframe) { RasterizerKey.FillMode = D3D11_FILL_WIREFRAME; }
    PipelineDescKey.RasterizerKey = RasterizerKey;
    Pipeline->UpdatePipeline(Pipeline->GetOrCreatePipelineState(PipelineDescKey));

    // Sampler
    UResourceManager& RM = UResourceManager::GetInstance();
    ID3D11SamplerState* Samp = RM.GetSamplerState(ESamplerType::Text);

    // Camera axes
    const FVector CamForward = Cam ? Cam->GetForward() : FVector(1,0,0);
    const FVector WorldUp = FVector(0,0,1);
    FVector Right = WorldUp.Cross(CamForward); Right.Normalize();
    FVector Up = CamForward.Cross(Right); Up.Normalize();

    struct FPosUv { FVector P; FVector2 UV; };

    // Sort back-to-front (transparent)
    struct FBItem { UBillboardComponent* C; float D; };
    TArray<FBItem> Sorted;
    Sorted.reserve(List.Num());
    for (auto* C : List)
    {
        FBItem I{ C, (CameraLocation - C->GetWorldLocation()).Length() };
        Sorted.push_back(I);
    }
    std::sort(Sorted.begin(), Sorted.end(), [](const FBItem& A, const FBItem& B){ return A.D > B.D; });

    for (const FBItem& It : Sorted)
    {
        UBillboardComponent* C = It.C;
        if (!C || !C->GetSpriteSRV()) continue;

        float W=0,H=0; {
            FAABB B = C->GetWorldBounds();
            FVector Ext = B.GetExtent();
            W = Ext.X * 2.f; H = Ext.Y * 2.f; if (W<=0 || H<=0) { W=1.f; H=1.f; }
        }
        const float HW = 0.5f * W; const float HH = 0.5f * H;
        const FVector Center = C->GetWorldLocation();

        const FVector P0 = Center + Right * (-HW)	+   Up * (-HH);
		const FVector P1 = Center + Right * (HW)	+	Up * (-HH);
		const FVector P2 = Center + Right * (HW)	+	Up * (HH);
		const FVector P3 = Center + Right * (-HW)	+	Up * (HH);

        float U=0,V=0,UL=1,VL=1; C->GetUV(U,V,UL,VL);

        TArray<FPosUv> Vtx;
        Vtx = {
            { P0, {U,     V+VL} },
            { P1, {U+UL,  V+VL} },
            { P2, {U+UL,  V    } },
            { P0, {U,     V+VL} },
            { P2, {U+UL,  V    } },
            { P3, {U,     V    } },
        };

        const uint32 VertexCount = static_cast<uint32>(Vtx.Num());
        const uint32 VertexStride = sizeof(FVector) + sizeof(FVector2);
        if (!UploadBillboardVertices(Vtx.data(), VertexCount, VertexStride))
        {
            continue;
        }

        // Constant buffers
        Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
        UpdateConstant(FModelConstant(FMatrix::Identity, C->GetInternalIndex()));

        // Billboard CB (b4)
        UpdateBillboardConstant(FVector4(U, V, UL, VL), C->GetColor());

        // Bind texture & sampler
        Pipeline->SetShaderResourceView(0, false, C->GetSpriteSRV());
        Pipeline->SetSamplerState(0, false, Samp);

        Pipeline->SetVertexBuffer(BillboardVertexBuffer, VertexStride);
        Pipeline->Draw(VertexCount, 0);
    }
}

bool URenderer::UploadBillboardVertices(const void* VertexData, uint32 VertexCount, uint32 Stride)
{
	if (VertexCount == 0 || Stride == 0)
	{
		return false;
	}

	EnsureBillboardVertexBuffer(VertexCount, Stride);
	if (BillboardVertexBuffer == nullptr)
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	HRESULT hr = GetDeviceContext()->Map(BillboardVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	if (FAILED(hr))
	{
		return false;
	}

	std::memcpy(Mapped.pData, VertexData, static_cast<size_t>(VertexCount) * static_cast<size_t>(Stride));
	GetDeviceContext()->Unmap(BillboardVertexBuffer, 0);
	return true;
}

void URenderer::EnsureBillboardVertexBuffer(uint32 VertexCount, uint32 Stride)
{
	if (VertexCount == 0 || Stride == 0)
	{
		return;
	}

	if (BillboardVertexBuffer && BillboardVertexBufferVertexCount >= VertexCount && BillboardVertexBufferStride == Stride)
	{
		return;
	}

	ReleaseVertexBuffer(BillboardVertexBuffer);
	BillboardVertexBuffer = nullptr;
	BillboardVertexBufferVertexCount = 0;
	BillboardVertexBufferStride = 0;

	D3D11_BUFFER_DESC Desc{};
	Desc.ByteWidth = VertexCount * Stride;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = GetDevice()->CreateBuffer(&Desc, nullptr, &BillboardVertexBuffer);
	if (FAILED(hr))
	{
		return;
	}

	BillboardVertexBufferVertexCount = VertexCount;
	BillboardVertexBufferStride = Stride;
}

void URenderer::RenderText(const FVector& CameraLocation)
{
	if (IsShowFlagEnabled(EEngineShowFlags::SF_BillboardText) == false) { return; }

	// Rebind char table (slot b4) because billboard pass may override it
	Pipeline->SetConstantBuffer(4, true, ConstantBufferCharTable);

	//shader, rasterizaer state, depth stencil state, input layout 설정///////////////////
	FPipelineDescKey PipelineDescKey;
	PipelineDescKey.BlendType = EBlendType::Transparent;
	PipelineDescKey.DepthStencilType = EDepthStencilType::Transparent;
	PipelineDescKey.ShaderType = EShaderType::TextShader;
	PipelineDescKey.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRasterizerKey RasterizerKey;
	RasterizerKey.CullMode = D3D11_CULL_NONE;
	RasterizerKey.FillMode = D3D11_FILL_SOLID;
	if (CurrentViewMode == EViewModeIndex::Wireframe)
	{
		RasterizerKey.CullMode = D3D11_CULL_NONE;
		RasterizerKey.FillMode = D3D11_FILL_WIREFRAME;
	}

	PipelineDescKey.RasterizerKey = RasterizerKey;

	Pipeline->UpdatePipeline(Pipeline->GetOrCreatePipelineState(PipelineDescKey));
	/////////////////////////////////////////////////////////////////////////////////////

	/////////////텍스처, 샘플러 설정////////////////////
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	ID3D11ShaderResourceView* Srv = ResourceManager.GetTexture("Asset/Font/Pretendard-Regular.dds");
	ID3D11SamplerState* SamplerState = ResourceManager.GetSamplerState(ESamplerType::Text);

	Pipeline->SetShaderResourceView(0, false, Srv);
	Pipeline->SetSamplerState(0, false, SamplerState);
	////////////텍스처, 샘플러 설정//////////////////////




	//text(외 투명한 물체)들은 블랜딩을 적용하기 위해서 zbuffer에 쓰기를 하지 않음, 그래서 뒤에 있는 물체가 앞에 있는 물체 위에 렌더링되는 현상이 벌어짐
	//그래서 zbuffer에 쓰지 않으면서 추가로 카메라로부터 거리순으로 정렬을 해서 멀리 있는 물체부터 그려줘야함.
	///////////////////////////////////Sorting//////////////////////////////////////////
	struct RenderObject
	{
		UTextRenderComponent* Component;
		float DistanceToCamera;

		bool operator<(const RenderObject& Other) const
		{
			return this->DistanceToCamera > Other.DistanceToCamera;
		}
	};

	TArray<RenderObject> RenderList;

	for (UTextRenderComponent* Component : GWorld->GetCurrentLevel()->GetTextComponentsToRender())
	{
		RenderObject Object;
		Object.Component = Component;
		Object.DistanceToCamera = (CameraLocation - Component->GetWorldLocation()).Length();
		RenderList.push_back(Object);
	}
	std::sort(RenderList.begin(), RenderList.end());
	/////////////////////////////////////////////////////////////////////////////////////



	/////////////////////////////////정렬된 리스트 순회하면서 렌더링//////////////////////
	for (RenderObject& Object : RenderList)
	{
		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);

		USceneComponent* RootComponent = Object.Component->GetOwner()->GetRootComponent();

		UpdateConstant(FModelConstant(Object.Component->GetWorldTransformMatrix(),Object.Component->GetInternalIndex()));


		Pipeline->SetVertexBuffer(Object.Component->GetVertexBuffer(), StrideTextVertex);

		Pipeline->Draw(Object.Component->GetVertexNum(), 0);
	}
	////////////////////////////////////////////////////////////////////////////////////

}

void URenderer::RenderEditorPrimitive(FEditorPrimitive& Primitive, const FPipelineDescKey PipelineDescKey)
{
    Pipeline->UpdatePipeline(Pipeline->GetOrCreatePipelineState(PipelineDescKey));

    Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
    UpdateConstant(Primitive.Location, Primitive.Rotation, Primitive.Scale);

    Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
    UpdateConstant(Primitive.Color);

    Pipeline->SetVertexBuffer(Primitive.Vertexbuffer, Stride);
    if (Primitive.bUseIndex && Primitive.IndexBuffer && Primitive.NumIndices > 0)
    {
        Pipeline->SetIndexBuffer(Primitive.IndexBuffer, DXGI_FORMAT_R32_UINT);
        Pipeline->DrawIndexed(Primitive.NumIndices, 0, 0);
    }
    else
    {
        Pipeline->Draw(Primitive.NumVertices, 0);
    }
}


/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::RenderEnd() const
{
	//Draw Overlay
	//UOverlayManager::GetInstance().RenderOverlay();
	//OverlayManager.RenderOverlay();
	GetSwapChain()->Present(0, 0); // 1: VSync 활성화
}

void URenderer::DrawSplitterOverlay() const
{
	if (CurrentLayout != EViewportLayout::Quad) return;
	DXGI_SWAP_CHAIN_DESC scd = {};
	GetSwapChain()->GetDesc(&scd);
	float W = (float)scd.BufferDesc.Width;
	float H = (float)scd.BufferDesc.Height;
	float x = std::clamp(VerticalRatio, 0.1f, 0.9f) * W;
	float y = std::clamp(HorizontalRatio, 0.1f, 0.9f) * H;
	ImU32 col = IM_COL32(220, 220, 220, 200);
	float thick = 2.0f;
	ImDrawList* dl = ImGui::GetForegroundDrawList();
	dl->AddLine(ImVec2(x, 0.0f), ImVec2(x, H), col, thick);
	dl->AddLine(ImVec2(0.0f, y), ImVec2(W, y), col, thick);
}



void URenderer::OnResize(uint32 InWidth, uint32 InHeight)
{
    if (!DeviceResources || !GetDevice() || !GetDeviceContext() || !GetSwapChain()) return;


	UOverlayManager::GetInstance().ReleaseBackBufferResources();

	DeviceResources->ReleaseFrameBuffer();
	DeviceResources->ReleaseDepthBuffer();
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	// ResizeBuffers 호출
	HRESULT hr = GetSwapChain()->ResizeBuffers(2, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr))
	{
		UE_LOG("OnResize Failed");
		return;
	}
	DeviceResources->UpdateViewport();

    DeviceResources->CreateFrameBuffer();
    DeviceResources->CreateDepthBuffer();
    // Recreate ID buffer to match new size
    DeviceResources->ReleaseIdBuffer();
    DeviceResources->CreateIdBuffer();

    ID3D11RenderTargetView* RenderTargetView = DeviceResources->GetRenderTargetView();
    ID3D11RenderTargetView* IdBufferRTV = DeviceResources->GetIdBufferRTV();
    ID3D11RenderTargetView* RenderTargetViews[] = { RenderTargetView, IdBufferRTV }; // 컬러 + ID
    GetDeviceContext()->OMSetRenderTargets(2, RenderTargetViews, DeviceResources->GetDepthStencilView());
	UOverlayManager::GetInstance().RecreateBackBufferResources();

	MSOC.Resize(InWidth, InHeight);

	CachedIdBuffer.resize(InWidth * InHeight);
}

/**
 * @brief Vertex Buffer 소멸 함수
 * @param InVertexBuffer
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	if (InVertexBuffer) { InVertexBuffer->Release(); }
}

void URenderer::LoadViewportLayout()
{
	const path PrimaryIni   = UPathManager::GetInstance().GetRootPath()   / "Editor.ini";     // 실행 파일 폴더
	const path SecondaryIni = UPathManager::GetInstance().GetConfigPath() / "editor.ini";     // 과거 호환(Asset/Config)

	// One-time migration: if only the legacy file exists, move it to the new canonical path and remove legacy
	auto FileExists = [](const path& P) -> bool
	{
		DWORD attrs = GetFileAttributesW(P.wstring().c_str());
		return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
	};
	if (!FileExists(PrimaryIni) && FileExists(SecondaryIni))
	{
		CopyFileW(SecondaryIni.wstring().c_str(), PrimaryIni.wstring().c_str(), FALSE);
		DeleteFileW(SecondaryIni.wstring().c_str());
	}

	char Buffer[64] = {};
	auto ReadKey = [&](const char* Key, char* OutBuf, DWORD OutSize) -> bool
	{
		OutBuf[0] = '\0';
		GetPrivateProfileStringA("Viewport", Key, "", OutBuf, OutSize, PrimaryIni.string().c_str());
		return OutBuf[0] != '\0';
	};
	// Layout mode (Single/Quad)
	if (ReadKey("Layout", Buffer, sizeof(Buffer)))
	{
		std::string L = Buffer;
		//if (L == "Quad")      SetViewportLayout(EViewportLayout::Quad);
		//else if (L == "Single") SetViewportLayout(EViewportLayout::Single);
	}
	// Split ratios
	if (ReadKey("VerticalRatio", Buffer, sizeof(Buffer)))
	{
		float R = static_cast<float>(atof(Buffer));
		SetVerticalRatio(std::clamp(R, 0.1f, 0.9f));
	}
	if (ReadKey("HorizontalRatio", Buffer, sizeof(Buffer)))
	{
		float R = static_cast<float>(atof(Buffer));
		SetHorizontalRatio(std::clamp(R, 0.1f, 0.9f));
	}
	// Viewport type mapping
	char TypeBuf[32] = {};
	auto ReadType = [&](const char* Key, EViewportType DefaultType)
	{
		if (!ReadKey(Key, TypeBuf, sizeof(TypeBuf)))
		{
			return DefaultType;
		}
		std::string S = TypeBuf;
		if (S == "Perspective") return EViewportType::Perspective;
		if (S == "Top")         return EViewportType::Top;
		if (S == "Right")       return EViewportType::Right;
		if (S == "Front")       return EViewportType::Front;
		return DefaultType;
	};
	// Rect 수집 순서: 0=TopLeft, 1=BottomLeft, 2=TopRight, 3=BottomRight
	// 기본 매핑: TL=Perspective, TR=Top, BL=Right, BR=Front
	ViewTypes[0] = ReadType("TopLeft", EViewportType::Perspective);
	ViewTypes[1] = ReadType("BottomLeft", EViewportType::Right);
	ViewTypes[2] = ReadType("TopRight", EViewportType::Top);
	ViewTypes[3] = ReadType("BottomRight", EViewportType::Front);
}

int URenderer::GetHoveredViewportIndex(float MouseX, float MouseY, FRect& OutRect)
{
	DXGI_SWAP_CHAIN_DESC scd = {};
	GetSwapChain()->GetDesc(&scd);

	if (CurrentLayout == EViewportLayout::Single)
	{
		FRect R{0, 0, (LONG)scd.BufferDesc.Width, (LONG)scd.BufferDesc.Height};
		OutRect = R;
		if (MouseX >= R.X && MouseX < (R.X + R.W) && MouseY >= R.Y && MouseY < (R.Y + R.H))
		{
			return 0;
		}
		return -1;
	}
	// Quad: compute rects from ratios
	FRect Rects[4] = {};
	float W = (float)scd.BufferDesc.Width;
	float H = (float)scd.BufferDesc.Height;
	float splitX = std::clamp(VerticalRatio, 0.1f, 0.9f) * W;
	float splitY = std::clamp(HorizontalRatio, 0.1f, 0.9f) * H;
	Rects[0] = {0,   0,   (LONG)splitX,       (LONG)splitY};
	Rects[1] = {0,   (LONG)splitY, (LONG)splitX, (LONG)(H - splitY)};
	Rects[2] = { (LONG)splitX, 0,   (LONG)(W - splitX), (LONG)splitY};
	Rects[3] = { (LONG)splitX, (LONG)splitY, (LONG)(W - splitX),   (LONG)(H - splitY)};

	for (int i = 0; i < 4; ++i)
	{
		const FRect& R = Rects[i];
		if (MouseX >= R.X && MouseX < (R.X + R.W) && MouseY >= R.Y && MouseY < (R.Y + R.H))
		{
			OutRect = R;
			return i;
		}
	}
	return -1;
}

void URenderer::ReadbackIdBuffer()
{
	ID3D11DeviceContext* Ctx = GetDeviceContext();
	DXGI_SWAP_CHAIN_DESC scd = {};
	GetSwapChain()->GetDesc(&scd);
	const LONG maxW = (LONG)scd.BufferDesc.Width;
	const LONG maxH = (LONG)scd.BufferDesc.Height;
	// GPU → CPU 스테이징으로 전체 복사
	Ctx->CopyResource(DeviceResources->GetIdStagingBuffer(), DeviceResources->GetIdBuffer());

	D3D11_MAPPED_SUBRESOURCE Mapped{};
	if (SUCCEEDED(Ctx->Map(DeviceResources->GetIdStagingBuffer(), 0, D3D11_MAP_READ, 0, &Mapped)))
	{
		const uint8_t* Src = static_cast<const uint8_t*>(Mapped.pData);
		const size_t RowPitch = Mapped.RowPitch;
		const size_t BytesPerRow = maxW * sizeof(uint32);
		const size_t NumPixels = maxW * maxH;

		CachedIdBuffer.resize(maxW * maxH);
		for (int Index = 0; Index < maxH; Index++)
		{
			// 대상 주소: CachedIdBuffer의 i번째 줄 시작
			uint8_t* pDest = reinterpret_cast<uint8_t*>(CachedIdBuffer.data()) + (Index * BytesPerRow);
			// 원본 주소: GPU 메모리의 i번째 줄 시작
			const uint8_t* pSourceRow = Src + (Index * RowPitch);
			// 한 줄의 실제 데이터만큼만 복사
			memcpy(pDest, pSourceRow, BytesPerRow);
		}

		Ctx->Unmap(DeviceResources->GetIdStagingBuffer(), 0);
	}
}

UPrimitiveComponent* URenderer::GetCollidedPrimitive(int MouseX, int MouseY) const
{
	const FRect viewportRect = UViewportManager::GetInstance().GetRoot()->GetRect();
	const LONG viewportRight = viewportRect.X + viewportRect.W;
	const LONG viewportBottom = viewportRect.Y + viewportRect.H;
	if (MouseX < viewportRect.X || MouseX >= viewportRight ||
		MouseY < viewportRect.Y || MouseY >= viewportBottom)
	{
		return nullptr;
	}

	DXGI_SWAP_CHAIN_DESC scd = {};
	GetSwapChain()->GetDesc(&scd);
	const LONG bufferWidth = static_cast<LONG>(scd.BufferDesc.Width);
	const LONG bufferHeight = static_cast<LONG>(scd.BufferDesc.Height);
	if (bufferWidth <= 0 || bufferHeight <= 0)
	{
		return nullptr;
	}

	const LONG clampedX = std::clamp<LONG>(MouseX, 0, bufferWidth - 1);
	const LONG clampedY = std::clamp<LONG>(MouseY, 0, bufferHeight - 1);
	const size_t index = static_cast<size_t>(clampedY) * static_cast<size_t>(bufferWidth) + static_cast<size_t>(clampedX);
	if (index >= CachedIdBuffer.size())
	{
		return nullptr;
	}

	return Cast<UPrimitiveComponent>(GUObjectArray[CachedIdBuffer[index]]);
}
void URenderer::RenderVisibleSort(TArray<UPrimitiveComponent*>& PrimToRender)
{
	const FVector CamPos = Cam->GetLocation();

	TArray<FDrawItem> Items;
	Items.reserve(PrimToRender.Num());

	PrimToRender.Sort([&](const UPrimitiveComponent* A, const UPrimitiveComponent* B)
		{
			const FAABB AB = A->GetWorldBounds();
			const FAABB BB = B->GetWorldBounds();
			const float aKey = FMath::Dist2(AB.GetCenter(), CamPos);
			const float bKey = FMath::Dist2(BB.GetCenter(), CamPos);
			return aKey > bKey;
		});


}

void URenderer::UpdateSplitDrag()
{
	const UInputManager& Input = UInputManager::GetInstance();
	DXGI_SWAP_CHAIN_DESC scd = {};
	GetSwapChain()->GetDesc(&scd);
	float W = (float)scd.BufferDesc.Width;
	float H = (float)scd.BufferDesc.Height;
	FPoint MP{ (LONG)Input.GetMousePosition().X, (LONG)Input.GetMousePosition().Y };
	bool LPressed = Input.IsKeyPressed(EKeyInput::MouseLeft);
	bool LDown    = Input.IsKeyDown(EKeyInput::MouseLeft);
	bool LRel     = Input.IsKeyReleased(EKeyInput::MouseLeft);
	static bool PrevDownSplit = false; // local edge-detect state
	bool JustPressed = LDown && !PrevDownSplit; // edge detect independent of InputManager::Update order

	float splitX = std::clamp(VerticalRatio, 0.1f, 0.9f) * W;
	float splitY = std::clamp(HorizontalRatio, 0.1f, 0.9f) * H;
	float half = SplitHotThickness * 0.5f;
	bool wasDragging = (bDragVertical || bDragHorizontal);
	if (!bDragVertical && !bDragHorizontal)
	{
		if (LPressed || JustPressed)
		{
			bool nearV = std::abs(MP.X - splitX) <= half;
			bool nearH = std::abs(MP.Y - splitY) <= half;
			// If pressing near the intersection, capture both axes simultaneously
			if (nearV && nearH) { bDragVertical = true; bDragHorizontal = true; }
			else if (nearV) { bDragVertical = true; bDragHorizontal = false; }
			else if (nearH) { bDragHorizontal = true; bDragVertical = false; }
		}
	}
	else
	{
		if (LDown)
		{

			// While dragging, keep the selected axis captured and update continuously
			if (bDragVertical)
			{
				VerticalRatio = std::clamp(MP.X / std::max(1.0f, W), 0.1f, 0.9f);
			}
			if (bDragHorizontal)
			{
				HorizontalRatio = std::clamp(MP.Y / std::max(1.0f, H), 0.1f, 0.9f);
			}
		}
		if (!LDown)
		{
			bDragVertical = false;
			bDragHorizontal = false;
		}
	}

	// If a drag just ended, persist the new ratios
	if (wasDragging && !LDown)
	{
		SaveViewportLayout();
		SaveMultiViewCameraSettings();
	}

	// update prev
	PrevDownSplit = LDown;
}

void URenderer::SaveViewportLayout() const
{
	// 실행 파일 폴더의 Editor.ini에 저장
	const path ConfigFilePath = UPathManager::GetInstance().GetRootPath() / "Editor.ini";
	// Layout mode
	WritePrivateProfileStringA("Viewport", "Layout", (GetViewportLayout() == EViewportLayout::Quad) ? "Quad" : "Single", ConfigFilePath.string().c_str());
	// Split ratios
	char Buf[64];
	snprintf(Buf, sizeof(Buf), "%.6f", std::clamp(VerticalRatio, 0.1f, 0.9f));
	WritePrivateProfileStringA("Viewport", "VerticalRatio", Buf, ConfigFilePath.string().c_str());
	snprintf(Buf, sizeof(Buf), "%.6f", std::clamp(HorizontalRatio, 0.1f, 0.9f));
	WritePrivateProfileStringA("Viewport", "HorizontalRatio", Buf, ConfigFilePath.string().c_str());
	// View types
	auto WriteType = [&](const char* Key, EViewportType T)
	{
		const char* V = "Perspective";
		switch (T)
		{
		case EViewportType::Top: V = "Top"; break;
		case EViewportType::Right: V = "Right"; break;
		case EViewportType::Front: V = "Front"; break;
		default: V = "Perspective"; break;
		}
		WritePrivateProfileStringA("Viewport", Key, V, ConfigFilePath.string().c_str());
	};
	WriteType("TopLeft", ViewTypes[0]);
	WriteType("BottomLeft", ViewTypes[1]);
	WriteType("TopRight", ViewTypes[2]);
	WriteType("BottomRight", ViewTypes[3]);
}

void URenderer::LoadMultiViewCameraSettings()
{
	// 뷰포트 타입에 따른 이름 매핑
	const char* ViewportNames[4] = { "Perspective", "Right", "Top", "Front" };

	// 각 뷰포트의 카메라 설정 로드
	for (int i = 0; i < 4; ++i)
	{
		if (ViewCameras[i])
		{
			UCamera::LoadMultiViewCameraState(ViewCameras[i], ViewportNames[i]);
		}
	}
}

void URenderer::SaveMultiViewCameraSettings() const
{
	// 뷰포트 타입에 따른 이름 매핑
	const char* ViewportNames[4] = { "Perspective", "Right", "Top", "Front" };

	// 각 뷰포트의 카메라 설정 저장
	for (int i = 0; i < 4; ++i)
	{
		if (ViewCameras[i])
		{
			UCamera::SaveMultiViewCameraState(ViewCameras[i], ViewportNames[i]);
		}
	}
}

void URenderer::CheckAndSaveCameraSettings()
{
	if (bCameraSaveRequested)
	{
		// DT 대신 타이머 매니저에서 델타 타임을 가져옴니다
		CameraSaveTimer += DT; // DT는 글로벌 매크로로 정의된 것으로 가정

		if (CameraSaveTimer >= CAMERA_SAVE_DELAY)
		{
			// 설정 저장 실행
			SaveMultiViewCameraSettings();

			// 타이머 및 플래그 초기화
			bCameraSaveRequested = false;
			CameraSaveTimer = 0.0f;

			// 디버그 로그 (선택사항)
			// UE_LOG("MultiView camera settings auto-saved");
		}
	}
}

/**
 * @brief 상수 버퍼 생성 함수
 */
void URenderer::CreateConstantBuffer()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	/**
	 * @brief 모델에 사용될 상수 버퍼 생성
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FModelConstant) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferModels);
	}

	/**
	 * @brief 색상 수정에 사용할 상수 버퍼
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferColor);
	}

	/**
	 * @brief 카메라에 사용될 상수 버퍼 생성
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FViewProjConstants) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferPerFrame);
	}

	/**
	 * @brief 폰트에 사용될 조회 테이블 상수 버퍼 생성
	 */
	{
		const TArray<FCharacterInfo>& CharTable = ResourceManager.GetCharInfos();
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FCharacterInfo) * CharTable.Num();
		ConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ConstantBufferDesc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA CharTableData = {};
		CharTableData.pSysMem = CharTable.data();
		GetDevice()->CreateBuffer(&ConstantBufferDesc, &CharTableData, &ConstantBufferCharTable);

		Pipeline->SetConstantBuffer(4, true, ConstantBufferCharTable);
	}

	// Billboard constant buffer (UVRect + Color)
	{
		struct C { FVector4 A; FVector4 B; };
		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = sizeof(C) + 0xF & 0xFFFFFFF0;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		GetDevice()->CreateBuffer(&Desc, nullptr, &ConstantBufferBillboard);
	}

	//UpdateInstanceDrawConstants(false, 0, 0);
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBufferModels)
	{
		ConstantBufferModels->Release();
		ConstantBufferModels = nullptr;
	}

	if (ConstantBufferColor)
	{
		ConstantBufferColor->Release();
		ConstantBufferColor = nullptr;
	}

	if (ConstantBufferPerFrame)
	{
		ConstantBufferPerFrame->Release();
		ConstantBufferPerFrame = nullptr;
	}

	if (ConstantBufferCharTable)
	{
		ConstantBufferCharTable->Release();
		ConstantBufferCharTable = nullptr;
	}

	if (ConstantBufferBillboard)
	{
		ConstantBufferBillboard->Release();
		ConstantBufferBillboard = nullptr;
	}
}

void URenderer::UpdateConstant(const FModelConstant& InModelConstant) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;
		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		FModelConstant* Constants = static_cast<FModelConstant*>(ConstantBufferMSR.pData);
		*Constants = InModelConstant;
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

/**
 * @brief 상수 버퍼 업데이트 함수
 * @param InOffset
 * @param InScale Ball Size
 */
void URenderer::UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FModelConstant* constants = (FModelConstant*)constantbufferMSR.pData;
		{
			(*constants).ModelMat = FMatrix::GetModelMatrix(InPosition, FVector::GetDegreeToRadian(InRotation), InScale);
			(*constants).UUID = 0;
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

void URenderer::UpdateConstant(const FViewProjConstants& InViewProjConstants) const
{
	Pipeline->SetConstantBuffer(1, false, ConstantBufferPerFrame);
	Pipeline->SetConstantBuffer(1, true, ConstantBufferPerFrame);

	if (ConstantBufferPerFrame)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FViewProjConstants* ViewProjectionConstants = (FViewProjConstants*)ConstantBufferMSR.pData;
		{
			ViewProjectionConstants->View = InViewProjConstants.View;
			ViewProjectionConstants->Projection = InViewProjConstants.Projection;
			ViewProjectionConstants->ViewModeIndex = static_cast<uint32>(CurrentViewMode);
		}
		GetDeviceContext()->Unmap(ConstantBufferPerFrame, 0);
	}
}

void URenderer::UpdateConstant(const FVector4& Color) const
{
	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);

	if (ConstantBufferColor)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FVector4* ColorConstants = (FVector4*)ConstantBufferMSR.pData;
		{
			ColorConstants->X = Color.X;
			ColorConstants->Y = Color.Y;
			ColorConstants->Z = Color.Z;
			ColorConstants->W = Color.W;
		}
		GetDeviceContext()->Unmap(ConstantBufferColor, 0);
	}
}

void URenderer::UpdateBillboardConstant(const FVector4& UVRect, const FVector4& Color) const
{
    if (!ConstantBufferBillboard) return;
    D3D11_MAPPED_SUBRESOURCE M = {};
    GetDeviceContext()->Map(ConstantBufferBillboard, 0, D3D11_MAP_WRITE_DISCARD, 0, &M);
    struct C { FVector4 A; FVector4 B; };
    C* Data = (C*)M.pData;
    Data->A = UVRect;
    Data->B = Color;
    GetDeviceContext()->Unmap(ConstantBufferBillboard, 0);
    Pipeline->SetConstantBuffer(4, true, ConstantBufferBillboard);
    Pipeline->SetConstantBuffer(4, false, ConstantBufferBillboard);
}

URenderer::FStructuredBufferResource& URenderer::GetOrCreateStructuredBuffer(UStaticMesh* InKey)
{
	return StaticMeshStructuredBuffers[InKey];
}

void URenderer::EnsureStructuredBufferCapacity(FStructuredBufferResource& InResource, uint32 InRequiredInstanceCount)
{
	if (InRequiredInstanceCount == 0)
	{
		return;
	}

	if (InResource.Capacity >= InRequiredInstanceCount && InResource.Buffer && InResource.ShaderResourceView)
	{
		return;
	}

	uint32 NewCapacity = InResource.Capacity > 0 ? InResource.Capacity : 64u;
	while (NewCapacity < InRequiredInstanceCount)
	{
		NewCapacity *= 2u;
	}

	if (InResource.ShaderResourceView)
	{
		InResource.ShaderResourceView->Release();
		InResource.ShaderResourceView = nullptr;
	}

	if (InResource.Buffer)
	{
		InResource.Buffer->Release();
		InResource.Buffer = nullptr;
	}

	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.ByteWidth = sizeof(FInstanceGPUData) * NewCapacity;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.StructureByteStride = sizeof(FInstanceGPUData);

	ID3D11Device* Device = GetDevice();
	HRESULT Hr = Device->CreateBuffer(&BufferDesc, nullptr, &InResource.Buffer);
	if (FAILED(Hr) || !InResource.Buffer)
	{
		UE_LOG("Failed to create instance structured buffer");
		InResource.Capacity = 0;
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SrvDesc.Buffer.FirstElement = 0;
	SrvDesc.Buffer.NumElements = NewCapacity;

	Hr = Device->CreateShaderResourceView(InResource.Buffer, &SrvDesc, &InResource.ShaderResourceView);
	if (FAILED(Hr) || !InResource.ShaderResourceView)
	{
		UE_LOG("Failed to create instance buffer SRV");
		InResource.Buffer->Release();
		InResource.Buffer = nullptr;
		InResource.Capacity = 0;
		return;
	}

	InResource.Capacity = NewCapacity;
}

void URenderer::UploadStructuredBufferData(FStructuredBufferResource& InResource, const void* InData,
	uint32 InInstanceCount)
{
	if (!InResource.Buffer || InInstanceCount == 0)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	HRESULT Hr = GetDeviceContext()->Map(InResource.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	if (FAILED(Hr))
	{
		UE_LOG("Failed to map instance buffer");
		return;
	}

	const size_t CopySize = sizeof(FInstanceGPUData) * static_cast<size_t>(InInstanceCount);
	memcpy(Mapped.pData, InData, CopySize);
	GetDeviceContext()->Unmap(InResource.Buffer, 0);
}

void URenderer::ReleasePrimitiveInstanceBuffers()
{
	for (auto& Pair : StaticMeshStructuredBuffers)
	{
		if (Pair.second.ShaderResourceView)
		{
			Pair.second.ShaderResourceView->Release();
			Pair.second.ShaderResourceView = nullptr;
		}

		if (Pair.second.Buffer)
		{
			Pair.second.Buffer->Release();
			Pair.second.Buffer = nullptr;
		}

		Pair.second.Capacity = 0;
	}

	StaticMeshStructuredBuffers.Empty();
}

