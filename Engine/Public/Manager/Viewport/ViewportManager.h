#pragma once
#include "Core/Object.h"

class SWindow;
class SSplitter;
class FAppWindow;
class UCamera;
class FViewport;
class FViewportClient;

UCLASS()
class UViewportManager : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UViewportManager, UObject)
	DECLARE_SINGLETON(UViewportManager)

public:
	void Initialize(FAppWindow* InWindow);
	void Update();
	void RenderOverlay();
	void Release();

	// 마우스 입력을 스플리터/윈도우 트리에 라우팅 (드래그/리사이즈 처리)
	void TickInput();

	// 레이아웃
	void BuildSingleLayout(int32 PromoteIdx = -1);
	void BuildFourSplitLayout();

	// 루트 접근
	void SetRoot(SWindow* InRoot) { Root = InRoot; }
	SWindow* GetRoot() const { return Root; }
	SWindow* GetQuadRoot() const { return QuadRoot; }


	// 리프 Rect 수집
	void GetLeafRects(TArray<FRect>& OutRects) const;

	// 현재 마우스가 위치한 뷰포트 인덱스(-1이면 없음)
	int32 GetViewportIndexUnderMouse() const;

	// 주어진 뷰포트 인덱스 기준으로 로컬 NDC 계산(true면 성공)
	bool ComputeLocalNDCForViewport(int32 Index, float& OutNdcX, float& OutNdcY) const;

	TArray<FViewport*>& GetViewports() { return Viewports; }
	TArray<FViewportClient*>& GetClients() { return Clients; }

	// 카메라 배열 접근자
	const TArray<UCamera*>& GetOrthographicCameras() const { return OrthoGraphicCameras; }
	const TArray<UCamera*>& GetPerspectiveCameras() const { return PerspectiveCameras; }

	// ViewportChange 상태 접근자
	//EViewportChange GetViewportChange() const { return ViewportChange; }
	//void SetViewportChange(EViewportChange InChange) { ViewportChange = InChange; }

	
	/**
	* @brief 뷰포트 레이아웃 전환 시, 현재 다중뷰포트 스플리터의 비율을 저장합니다.
	* 
	*/
	void PersistSplitterRatios();


	void QuadToSingleAnimation();
	void SingleToQuadAnimation();

	// 애니메이션 시스템 공개 인터페이스
	bool IsAnimating() const { return ViewportAnimation.bIsAnimating; }
	void StartLayoutAnimation(bool bSingleToQuad, int32 ViewportIndex = -1);


	EViewportLayout GetViewportLayout()const;
	void SetViewportLayout(EViewportLayout InViewportLayout);

	void SetActiveIndex(int32 InActiveIndex)
	{
		ActiveIndex = InActiveIndex;
	}

	int32 GetActiveIndex()const
	{
		return ActiveIndex;
	}


	FRect GetActiveViewportRect()const
	{
		return ActiveViewportRect;
	}

	void SetActiveViewportRect(FRect InActiveViewportRect)
	{
		ActiveViewportRect = InActiveViewportRect;
	}


private:
	// 내부 유틸
	void SyncRectsToViewports() const; // 리프Rect → Viewport.Rect
	void SyncAnimationRectsToViewports() const; // 애니메이션 중 리프Rect → Viewport.Rect
	void PumpAllViewportInput() const; // 각 뷰포트 → 클라 입력 전달
	void TickCameras() const; // 카메라 업데이트 일원화(공유 오쇼 1회)
	void UpdateActiveRmbViewportIndex(); // 우클릭 드래그 대상 뷰포트 인덱스 계산

	void InitializeViewportAndClient();
	void InitializeOrthoGraphicCamera();
	void InitializePerspectiveCamera();

	void UpdateOrthoGraphicCameraPoint();

	void UpdateOrthoGraphicCameraFov() const;

	void BindOrthoGraphicCameraToClient() const;

	void ForceRefreshOrthoViewsAfterLayoutChange();

	void ApplySharedOrthoCenterToAllCameras() const;



	// 애니메이션 시스템 내부 함수들
	void StartViewportAnimation(bool bSingleToQuad, int32 PromoteIdx = -1);
	void UpdateViewportAnimation();
	float EaseInOutCubic(float t) const;
	void CreateAnimationSplitters();
	void AnimateSplitterTransition(float Progress);
	void RestoreOriginalLayout();
	void FinalizeSingleLayoutFromAnimation();
	void FinalizeFourSplitLayoutFromAnimation();


private:
	SWindow* Root = nullptr;
	SWindow* QuadRoot = nullptr;
	SWindow* Leaves[4] = { nullptr, nullptr, nullptr, nullptr };

	FRect ActiveViewportRect;

	SWindow* Capture = nullptr;

	// App main window for querying current client size each frame
	FAppWindow* AppWindow = nullptr;

	TArray<FViewport*> Viewports;
	TArray<FViewportClient*> Clients;

	TArray<UCamera*> OrthoGraphicCameras;
	TArray<UCamera*> PerspectiveCameras;

	TArray<FVector> InitialOffsets;

	FVector OrthoGraphicCamerapoint{ 0.0f, 0.0f, 0.0f };

	// 현재 우클릭(카메라 제어) 입력이 적용될 뷰포트 인덱스 (-1이면 없음)
	int32 ActiveRmbViewportIdx = -1;

	// 렌더 타입
	EViewportLayout ViewportLayout = EViewportLayout::Single;

	// 활성뷰포트 인덱스
	int32 ActiveIndex = 0;

	//EViewportChange ViewportChange = EViewportChange::Single;

	float SharedFovY = 150.0f;
	float SharedY = 0.5f;

	float IniSaveSharedV = 0.5f;
	float IniSaveSharedH = 0.5f;

	float SplitterValueV = 0.5f;
	float SplitterValueH = 0.5f;

	int32 LastPromotedIdx = -1;

	float ViewLayoutChangeSplitterH = 0.5f;
	float ViewLayoutChangeSplitterV = 0.5f;

	//스플리터 포인터
	SSplitter* SplitterV = nullptr;
	SSplitter* LeftSplitterH = nullptr;
	SSplitter* RightSplitterH = nullptr;

	// swindow 포인터
	SWindow* LeftTopWindow = nullptr;
	SWindow* LeftBottomWindow = nullptr;
	SWindow* RightTopWindow = nullptr;
	SWindow* RightBottomWindow = nullptr;

	// Viewport Animation System
	struct FViewportAnimation
	{
		bool bIsAnimating = false;
		float AnimationTime = 0.0f;
		float AnimationDuration = 0.2f; // 애니메이션 지속 시간 (초)

		bool bSingleToQuad = true; // true: Single→Quad, false: Quad→Single

		// SWindow 트리 백업 및 복원을 위한 데이터
		SWindow* BackupRoot = nullptr;
		SWindow* AnimationRoot = nullptr; // 애니메이션용 임시 루트

		// 스플리터 비율 정보
		float CurrentVerticalRatio = 0.5f;   // 현재 수직 스플리터 비율
		float CurrentHorizontalRatio = 0.5f; // 현재 수평 스플리터 비율
		float StartRatio = 0.5f;  // 시작 스플리터 비율
		float TargetRatio = 0.5f; // 목표 스플리터 비율
		int32 PromotedViewportIndex = 0;

		// UViewportManager::FViewportAnimation 에 아래 필드 추가
		float StartVRatio = 0.5f;
		float StartHRatio = 0.5f;
		float TargetVRatio = 0.5f;
		float TargetHRatio = 0.5f;
		int   SavedMinChildSizeV = 4;
		int   SavedMinChildSizeH = 4;
	};

	FViewportAnimation ViewportAnimation;

};

