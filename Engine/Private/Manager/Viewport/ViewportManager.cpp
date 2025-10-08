#include "pch.h"
#include "public/Manager/Viewport/ViewportManager.h"
#include "Render/UI/Layout/Window.h"
#include "Core/AppWindow.h"
#include "Render/Viewport/Viewport.h"
#include "Public/Render/Viewport/ViewportClient.h"
#include "Manager/UI/UIManager.h"
#include "Render/UI/Layout/Splitter.h"
#include "Render/UI/Layout/SplitterV.h"
#include "Render/UI/Layout/SplitterH.h"
#include "public/Render/UI/Window/LevelTabBarWindow.h"
#include "public/Render/UI/Window/MainMenuWindow.h"
#include "Public/Manager/Input/InputManager.h"

IMPLEMENT_CLASS(UViewportManager, UObject)

IMPLEMENT_SINGLETON(UViewportManager)

UViewportManager::UViewportManager() = default;
UViewportManager::~UViewportManager()
{
	SafeDelete(SplitterV);

	SafeDelete(LeftSplitterH);
	SafeDelete(RightSplitterH);

	SafeDelete(LeftTopWindow);
	SafeDelete(LeftBottomWindow);
	SafeDelete(RightTopWindow);
	SafeDelete(RightBottomWindow);
}

void UViewportManager::Initialize(FAppWindow* InWindow)
{
	// 밖에서 윈도우를 가져와 크기를 가져온다
	int32 Width = 0, Height = 0;
	if (InWindow)
	{
		InWindow->GetClientSize(Width, Height);
	}
	AppWindow = InWindow;

	// 루트 윈도우에 새로운 윈도우를 할당합니다.
	ViewportLayout = EViewportLayout::Quad;

	// 0번 인덱스의 뷰포트로 초기화
	ActiveIndex = 0;

	float MenuAndLevelBarHeight = ULevelTabBarWindow::GetInstance().GetLevelBarHeight() + UMainMenuWindow::GetInstance().GetMenuBarHeight();

	//ActiveViewportRect = { 0, (LONG)MenuAndLevelBarHeight, Width, Height - (LONG)MenuAndLevelBarHeight };

	// 뷰포트 슬롯 최대치까진 준비(포인터만). 아직 RT/DSV 없음.
	// 4개의 뷰포트, 클라이언트, 카메라 를 할당받습니다.
	InitializeViewportAndClient();


	//IniSaveSharedV = UConfigManager::GetInstance().GetSplitV();
	//IniSaveSharedH = UConfigManager::GetInstance().GetSplitH();


	// 부모 스플리터
	SplitterV = new SSplitterV();
	SplitterV->Ratio = IniSaveSharedV;

	// 자식 스플리터
	LeftSplitterH = new SSplitterH();
	RightSplitterH = new SSplitterH();

	// 부모 스플리터의 자식 스플리터를 셋합니다.
	SplitterV->SetChildren(LeftSplitterH, RightSplitterH);

	// 자식 스플리터에 붙을 윈도우를 셋합니다.
	LeftTopWindow		= new SWindow();
	LeftBottomWindow	= new SWindow();
	RightTopWindow		= new SWindow();
	RightBottomWindow	= new SWindow();

	// 자식 스플리터에 자식 윈도우를 셋합니다.
	LeftSplitterH->SetChildren(LeftTopWindow, LeftBottomWindow);
	RightSplitterH->SetChildren(RightTopWindow, RightBottomWindow);

	// 리프 배열로 고정 매핑
	Leaves[0] = LeftTopWindow;
	Leaves[1] = LeftBottomWindow;
	Leaves[2] = RightTopWindow;
	Leaves[3] = RightBottomWindow;

	QuadRoot = SplitterV;

	// 싱글: Root를 선택된 리프로 바꾸고 그 리프만 크게
	Root = Leaves[ActiveIndex];
	Root->OnResize(ActiveViewportRect);

	SplitterValueV = IniSaveSharedV;
	SplitterValueH = IniSaveSharedH;
}

void UViewportManager::Update()
{
	if (!Root)
	{
		return;
	}

	int32 Width = 0, Height = 0;

	// AppWindow는 실제 윈도우의 메세지콜이나 크기를 담당합니다
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}

	// 91px height
	const int MenuAndLevelHeight = UMainMenuWindow::GetInstance().GetMenuBarHeight() + ULevelTabBarWindow::GetInstance().GetLevelBarHeight();


	// ActiveViewportRect는 실제로 렌더링이 될 영역의 뷰포트 입니다
	const int32 RightPanelWidth = static_cast<int32>(UUIManager::GetInstance().GetRightPanelWidth());
	const int32 ViewportWidth = Width - RightPanelWidth;

	ActiveViewportRect = FRect{ 0, MenuAndLevelHeight, max(0, ViewportWidth), max(0, Height - MenuAndLevelHeight) };


	if (ViewportLayout == EViewportLayout::Quad)
	{
	    if (QuadRoot)
	    {
	        QuadRoot->OnResize(ActiveViewportRect); // FourSplit일 때만 실행됨
	    }
	}
	else
	{
		if (Root)
		{
			Root->OnResize(ActiveViewportRect);
		}
	}


	// 스플리터 드래그 처리 함수
	TickInput();



	// 4) 드로우(3D) — 실제 렌더러 루프에서 Viewport 적용 후 호출해도 됨
	//    여기서는 뷰/클라 페어 순회만 보여줌. (RS 바인딩은 네 렌더러 Update에서 수행 중)
	const int32 N = static_cast<int32>(Clients.size());
	for (int32 i = 0; i < 1; ++i)
	{
		Clients[i]->Draw(Viewports[i]);
	}
}


void UViewportManager::RenderOverlay()
{
	if (!QuadRoot)
	{
		return;
	}
	// 스플리터 선만 렌더링 (나머지 UI는 ViewportControlWidget에서 처리)
	QuadRoot->OnPaint();
}

void UViewportManager::Release()
{
}

void UViewportManager::TickInput()
{
	if (!QuadRoot)
	{
		return;
	}

	auto& InputManager = UInputManager::GetInstance();
	const FVector& MousePosition = InputManager.GetMousePosition();
	FPoint Point{ static_cast<LONG>(MousePosition.X), static_cast<LONG>(MousePosition.Y) };

	SWindow* Target = nullptr;

	if (Capture != nullptr)
	{
		// 드래그 캡처 중이면, 히트 테스트 없이 캡처된 위젯으로 고정
		Target = static_cast<SWindow*>(Capture);
	}
	else
	{
		// 캡처가 아니면 화면 좌표로 히트 테스트
		Target = (QuadRoot != nullptr) ? QuadRoot->HitTest(Point) : nullptr;
	}

	if (InputManager.IsKeyPressed(EKeyInput::MouseLeft) || (!Capture && InputManager.IsKeyDown(EKeyInput::MouseLeft)))
	{
		if (Target && Target->OnMouseDown(Point, 0))
		{
			Capture = Target;
		}
	}

	const FVector& Delta = InputManager.GetMouseDelta();
	if ((Delta.X != 0.0f || Delta.Y != 0.0f) && Capture)
	{
		Capture->OnMouseMove(Point);
	}

	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		if (Capture)
		{
			Capture->OnMouseUp(Point, 0);
			Capture = nullptr;
		}
	}

	if (!InputManager.IsKeyDown(EKeyInput::MouseLeft) && Capture)
	{
		Capture->OnMouseUp(Point, 0);
		Capture = nullptr;
	}
}

void UViewportManager::BuildSingleLayout(int32 PromoteIdx)
{
}

void UViewportManager::BuildFourSplitLayout()
{
}

void UViewportManager::GetLeafRects(TArray<FRect>& OutRects) const
{
	OutRects.clear();
	if (!Root)
	{
		return;
	}

	// 재귀 수집
	struct Local
	{
		static void Collect(SWindow* Node, TArray<FRect>& Out)
		{
			if (auto* Split = Cast(Node))
			{
				Collect(Split->SideLT, Out);
				Collect(Split->SideRB, Out);
			}
			else
			{
				Out.emplace_back(Node->GetRect());
			}
		}
	};
	Local::Collect(Root, OutRects);
}

int32 UViewportManager::GetViewportIndexUnderMouse() const
{
    return int32();
}

bool UViewportManager::ComputeLocalNDCForViewport(int32 Index, float& OutNdcX, float& OutNdcY) const
{
    return false;
}

void UViewportManager::PersistSplitterRatios()
{
}

void UViewportManager::StartLayoutAnimation(bool bSingleToQuad, int32 ViewportIndex)
{
}

void UViewportManager::SyncRectsToViewports() const
{
}

void UViewportManager::SyncAnimationRectsToViewports() const
{
}

void UViewportManager::PumpAllViewportInput() const
{
}

void UViewportManager::TickCameras() const
{
}

void UViewportManager::UpdateActiveRmbViewportIndex()
{
}

void UViewportManager::InitializeViewportAndClient()
{
	for (int i = 0; i < 4; i++)
	{
		FViewport* Viewport = new FViewport();
		FViewportClient* ViewportClient = new FViewportClient();

		Viewport->SetViewportClient(ViewportClient);

		Clients.push_back(ViewportClient);
		Viewports.push_back(Viewport);
	}


	// 이 값들은 조절가능 일단 기본으로 셋
	Clients[0]->SetViewType(EViewType::Perspective);
	Clients[0]->SetViewMode(EViewMode::Unlit);

	Clients[1]->SetViewType(EViewType::OrthoLeft);
	Clients[1]->SetViewMode(EViewMode::WireFrame);

	Clients[2]->SetViewType(EViewType::OrthoTop);
	Clients[2]->SetViewMode(EViewMode::WireFrame);

	Clients[3]->SetViewType(EViewType::OrthoRight);
	Clients[3]->SetViewMode(EViewMode::WireFrame);
}

void UViewportManager::InitializeOrthoGraphicCamera()
{
}

void UViewportManager::InitializePerspectiveCamera()
{
}

void UViewportManager::UpdateOrthoGraphicCameraPoint()
{
}

void UViewportManager::UpdateOrthoGraphicCameraFov() const
{
}

void UViewportManager::BindOrthoGraphicCameraToClient() const
{
}

void UViewportManager::ForceRefreshOrthoViewsAfterLayoutChange()
{
}

void UViewportManager::ApplySharedOrthoCenterToAllCameras() const
{
}

EViewportLayout UViewportManager::GetViewportLayout() const
{
	return ViewportLayout;
}

void UViewportManager::SetViewportLayout(EViewportLayout InViewportLayout)
{
	ViewportLayout = InViewportLayout;
}

void UViewportManager::StartViewportAnimation(bool bSingleToQuad, int32 PromoteIdx)
{
}

void UViewportManager::UpdateViewportAnimation()
{
}

float UViewportManager::EaseInOutCubic(float t) const
{
    return 0.0f;
}

void UViewportManager::CreateAnimationSplitters()
{
}

void UViewportManager::AnimateSplitterTransition(float Progress)
{
}

void UViewportManager::RestoreOriginalLayout()
{
}

void UViewportManager::FinalizeSingleLayoutFromAnimation()
{
}

void UViewportManager::FinalizeFourSplitLayoutFromAnimation()
{
}
