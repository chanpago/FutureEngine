#pragma once
#include "DeviceResources.h"
#include "Core/Object.h"
#include "Components/SceneComponent.h"
#include "Editor/EditorPrimitive.h"
#include "Render/UI/Layout/Window.h"
#include "Render/UI/Layout/MultiViewBuilders.h"
#include "Editor/Camera.h"
#include "Math/Octree.h"

#include "Render/Cull/MSOC.h"
class UPipeline;
class UDeviceResources;
class UPrimitiveComponent;
class UStaticMesh;
class AActor;
class AGizmo;
class UEditor;
struct FPipelineInfo;
/**
 * @brief Rendering Pipeline 전반을 처리하는 클래스
 *
 * Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
 * @param Device GPU와 통신하기 위한 Direct3D 장치
 * @param DeviceContext GPU 명령 실행을 담당하는 컨텍스트
 * @param SwapChain 프레임 버퍼를 교체하는 데 사용되는 스왑 체인
 *
 * // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
 * @param FrameBuffer 화면 출력용 텍스처
 * @param FrameBufferRTV 텍스처를 렌더 타겟으로 사용하는 뷰
 * @param RasterizerState 래스터라이저 상태(컬링, 채우기 모드 등 정의)
 * @param ConstantBuffer 쉐이더에 데이터를 전달하기 위한 상수 버퍼
 *
 * @param ClearColor 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
 * @param ViewportInfo 렌더링 영역을 정의하는 뷰포트 정보
 *
 * @param DefaultVertexShader
 * @param DefaultPixelShader
 * @param DefaultInputLayout
 * @param Stride
 *
 * @param vertexBufferSphere
 * @param numVerticesSphere
 */

struct FDrawItem
{
	UPrimitiveComponent* Comp;
	float Key;
};

struct FModelConstant
{
	FMatrix ModelMat;
	uint32 UUID;
	float Pad[3];
};

class URenderer : public UObject
{
	DECLARE_CLASS(URenderer, UObject)
	DECLARE_SINGLETON(URenderer)


public:
	void Init(HWND InWindowHandle);
	void Release();

	void CreateConstantBuffer();

	static void ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer);
	void ReleaseConstantBuffer();
	
	void ReleaseResource();

	void Update(UEditor* Editor);
	void RenderBegin();
	void RenderLevel();
	void RenderBillboards(const FVector& CameraLocation);
	void RenderText(const FVector& CameraLocation);
	void RenderEditorPrimitive(FEditorPrimitive& Primitive, const FPipelineDescKey PipelineDescKey);
	void RenderEnd() const;

// Layout control
	

	EViewportLayout GetViewportLayout() const { return CurrentLayout; }

	// Viewport hit-testing and camera access
	int GetHoveredViewportIndex(float MouseX, float MouseY, FRect& OutRect);
	UCamera* GetViewCameraAt(int Index) const { return (Index >= 0 && Index < 4) ? ViewCameras[Index] : nullptr; }
	EViewportType GetViewportTypeAt(int Index) const { return (Index >= 0 && Index < 4) ? ViewTypes[Index] : EViewportType::Perspective; }
	// Current render viewport type (used by grid/axis orientation)
	EViewportType GetCurrentRenderViewportType() const { return CurrentRenderVType; }
	void SetCurrentRenderViewportType(EViewportType InT) { CurrentRenderVType = InT; }

	


	// Two-axis split ratios (0..1) for Quad layout
	float GetVerticalRatio() const { return VerticalRatio; }
	float GetHorizontalRatio() const { return HorizontalRatio; }
	void SetVerticalRatio(float R) { VerticalRatio = std::clamp(R, 0.1f, 0.9f); }
	void SetHorizontalRatio(float R) { HorizontalRatio = std::clamp(R, 0.1f, 0.9f); }

	// Drag state for split lines
	bool IsDraggingSplitter() const { return bDragVertical || bDragHorizontal; }

	// UI overlay for splitter handles
	void DrawSplitterOverlay() const;


	void OnResize(uint32 Inwidth = 0, uint32 InHeight = 0);
	bool GetIsResizing() { return bIsResizing; }
	void SetIsResizing(bool isResizing) { bIsResizing = isResizing; }

	// Viewport layout save/load (editor.ini)
	void LoadViewportLayout();
	void SaveViewportLayout() const;

	// MultiView camera settings save/load (editor.ini)
	void LoadMultiViewCameraSettings();
	void SaveMultiViewCameraSettings() const;

	// Auto-save camera settings when changed
	void CheckAndSaveCameraSettings();
	void RequestCameraSave() { bCameraSaveRequested = true; }


	template<typename T>
	ID3D11Buffer* CreateVertexBuffer(TArray<T>& InVertices) const
	{
		D3D11_BUFFER_DESC VertexBufferDesc = {};
		VertexBufferDesc.ByteWidth = InVertices.size() * sizeof(T);
		VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated
		VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA VertexBufferSRD = { InVertices.data() };

		ID3D11Buffer* VertexBuffer;

		GetDevice()->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &VertexBuffer);

		return VertexBuffer;
	}

	template<typename T>
	ID3D11Buffer* CreateIndexBuffer(TArray<T>& InIndices) const
	{
		D3D11_BUFFER_DESC IndexBufferDesc = {};
		IndexBufferDesc.ByteWidth = InIndices.size() * sizeof(T);
		IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated
		IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA IndexBufferSRD = { InIndices.data() };

		ID3D11Buffer* IndexBuffer;

		GetDevice()->CreateBuffer(&IndexBufferDesc, &IndexBufferSRD, &IndexBuffer);

		return IndexBuffer;
	}


	void UpdateConstant(const FModelConstant& InModelConstant) const;
	void UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const;
	void UpdateConstant(const FViewProjConstants& InViewProjConstants) const;
	void UpdateConstant(const FVector4& Color) const;
	void UpdateBillboardConstant(const FVector4& UVRect, const FVector4& Color) const;

	void SetViewMode(EViewModeIndex InViewMode) { CurrentViewMode = InViewMode; }
	void SetOrthoWorldWidthConst(float InWidth) { OrthoWidthConst = InWidth; }
	EViewModeIndex GetViewMode(EViewModeIndex InViewMode) const { return CurrentViewMode; }

	/** Show Flags management */
	void SetShowFlags(EEngineShowFlags InShowFlags) { CurrentShowFlags = InShowFlags; }
	EEngineShowFlags GetShowFlags() const { return CurrentShowFlags; }
	void ToggleShowFlag(EEngineShowFlags InFlag) { CurrentShowFlags = CurrentShowFlags ^ InFlag; }
	bool IsShowFlagEnabled(EEngineShowFlags InFlag) const { return HasFlag(CurrentShowFlags, InFlag); }

	ID3D11Device* GetDevice() const { return DeviceResources->GetDevice(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceResources->GetDeviceContext(); }
	IDXGISwapChain* GetSwapChain() const { return DeviceResources->GetSwapChain(); }
	ID3D11RenderTargetView* GetRenderTargetView() const { return DeviceResources->GetRenderTargetView(); }
	UDeviceResources* GetDeviceResources() const { return DeviceResources; }
	UPrimitiveComponent* GetCollidedPrimitive(int MouseX, int MouseY) const;

	/** LineBatchRenderer에서 사용할 공개 메서드 */
	UPipeline* GetPipeline() const { return Pipeline; }
	float GetOrthoWorldWidthConst(){ return OrthoWidthConst; }

	USoftwareOcclusionCuller MSOC;

	void RenderVisibleSort(TArray<UPrimitiveComponent*>& PrimToRender);


	void ReadbackIdBuffer();

private:
	//MSOC viewprot
	D3D11_VIEWPORT PerspectiveViewport;
private:
	void UpdateSplitDrag();

private:
	UPipeline* Pipeline = nullptr;
	UDeviceResources* DeviceResources = nullptr;
	EViewModeIndex CurrentViewMode = EViewModeIndex::Lit;
	EEngineShowFlags CurrentShowFlags = EEngineShowFlags::SF_Default;
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	TArray<UPrimitiveComponent*> PrimitiveComponentsToRender;

	// Multiview root splitter (no longer used for layout math, kept for compatibility)
	SWindow* MultiViewRoot = nullptr;
	EViewportLayout CurrentLayout = EViewportLayout::Single; // default: single view on start

	// Two split ratios and drag flags
	float OrthoWidthConst = 30.0f;
	float VerticalRatio = 0.5f;   // 0..1, X split
	float HorizontalRatio = 0.5f; // 0..1, Y split
	bool  bDragVertical = false;
	bool  bDragHorizontal = false;
	bool  bMouseLeftDownPrevSplit = false; // internal edge detect for split dragging
	static constexpr float SplitHotThickness = 8.0f; // px

	// Per-viewport cameras and types
	bool bDraggingSplitter = false;

	// Per-viewport cameras and types
	UCamera* ViewCameras[4] = { nullptr, nullptr, nullptr, nullptr };
	// 0=TL(Persp),1=BL(Right),2=TR(Top),3=BR(Front)
	EViewportType ViewTypes[4] = { EViewportType::Perspective, EViewportType::Right, EViewportType::Top, EViewportType::Front };
	EViewportType CurrentRenderVType = EViewportType::Perspective;

	// Camera auto-save system
	bool bCameraSaveRequested = false;
	float CameraSaveTimer = 0.0f;
	static constexpr float CAMERA_SAVE_DELAY = 2.0f; // 2초 후 자동 저장



	UCamera* Cam;
private:
	ID3D11DepthStencilState* DefaultDepthStencilState = nullptr;
	ID3D11DepthStencilState* DisabledDepthStencilState = nullptr;
	ID3D11DepthStencilState* TextDepthStencilState = nullptr;
	ID3D11BlendState* TextBlendState = nullptr;
	ID3D11Buffer* ConstantBufferModels = nullptr;
	ID3D11Buffer* ConstantBufferPerFrame = nullptr;
	ID3D11Buffer* ConstantBufferColor = nullptr;
	ID3D11Buffer* ConstantBufferCharTable = nullptr;
	ID3D11Buffer* ConstantBufferBillboard = nullptr;


	std::vector<uint32> CachedIdBuffer;
	/////////////////////////////////////
	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };



	uint32 StrideStaticMesh = sizeof(FNormalVertex);

	uint32 Stride = sizeof(FVertex);
	uint32 StrideTextVertex = sizeof(FTextVertex);



	struct FStructuredBufferResource
	{
		ID3D11Buffer* Buffer = nullptr;
		ID3D11ShaderResourceView* ShaderResourceView = nullptr;	//버퍼에 대한 뷰
		uint32 Capacity = 0;	//스트럭처드 버퍼 크기
	};

	TMap<UStaticMesh*, FStructuredBufferResource> StaticMeshStructuredBuffers;
private:

	FStructuredBufferResource& GetOrCreateStructuredBuffer(UStaticMesh* InKey);
	void EnsureStructuredBufferCapacity(FStructuredBufferResource& InResource, uint32 InRequiredInstanceCount);
	void UploadStructuredBufferData(FStructuredBufferResource& InResource, const void* InData, uint32 InInstanceCount);
	void ReleasePrimitiveInstanceBuffers();
	void OctreeFrustumCulling(const TArray<TOctree<UPrimitiveComponent, PrimitiveComponentTrait>::FOctreeNode>& OctreeNodeList,
		const TArray<UPrimitiveComponent*>& OctreeElementList,
		TArray<UPrimitiveComponent*>& PrimitiveComponentsToRender,
		uint32 CurrentNodeIndex);

	bool bIsResizing = false; 
	UWorld* CacheWorld;
};
