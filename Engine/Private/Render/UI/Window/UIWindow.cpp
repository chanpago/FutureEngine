#include "pch.h"
#include "Render/UI/Window/UIWindow.h"
#include "Render/UI/Layout/Window.h"
#include "ImGui/imgui_internal.h"
#include "public/Manager/Viewport/ViewportManager.h"
#include "Render/UI/Widget/Widget.h"
#include "Manager/UI/UIManager.h"

IMPLEMENT_ABSTRACT_CLASS(UUIWindow, UObject);

int UUIWindow::IssuedWindowID = 0;

UUIWindow::UUIWindow(const FUIWindowConfig& InConfig) : Config(InConfig), CurrentState(InConfig.InitialState)
{
	// 고유한 윈도우 ID 생성
	WindowID = ++IssuedWindowID;

	// 윈도우 플래그 업데이트
	Config.UpdateWindowFlags();

	// 초기 상태 설정
	LastWindowSize = Config.DefaultSize;
	LastWindowPosition = Config.DefaultPosition;

	if (IssuedWindowID == 1)
	{
		cout << "UIWindow: Created: " << WindowID << " (" << Config.WindowTitle << ")" << "\n";
	}
	else
	{
		UE_LOG("UIWindow: Created: %d (%s)", WindowID, Config.WindowTitle.c_str());
	}


}

UUIWindow::~UUIWindow()
{
	for (UWidget* Widget : Widgets)
	{
		SafeDelete(Widget);
	}
}

/**
 * @brief 뷰포트가 리사이징 되었을 때 앵커/좌상단 기준 상대 위치 비율을 고정하는 로직
 */
void UUIWindow::OnMainWindowResized()
{
	if (!ImGui::GetCurrentContext() || !IsVisible())
		return;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 currentViewportSize = viewport->WorkSize;

 	const ImVec2 anchor = PositionRatio;
	const ImVec2 pivot = { 0.f, 0.f };

	ImVec2 responsiveSize(
		currentViewportSize.x * SizeRatio.x,
		currentViewportSize.y * SizeRatio.y
	);

	ImVec2 targetPos(
		viewport->WorkPos.x + currentViewportSize.x * anchor.x,
		viewport->WorkPos.y + currentViewportSize.y * anchor.y
	);

	ImVec2 finalPos(
		targetPos.x - responsiveSize.x * pivot.x,
		targetPos.y - responsiveSize.y * pivot.y
	);

	ImGui::SetWindowSize(responsiveSize, ImGuiCond_Always);
	ImGui::SetWindowPos(finalPos, ImGuiCond_Always);
}

/**
 * @brief 윈도우가 뷰포트 범위 밖으로 나갔을 시 클램프 하는 로직
 */
void UUIWindow::ClampWindow()
{
	if (!IsVisible())
	{
		return;
	}

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	const ImVec2 WorkPos = Viewport->WorkPos;
	const ImVec2 WorkSize = Viewport->WorkSize;

	ImVec2 pos = LastWindowPosition;
	ImVec2 size = LastWindowSize;

	bool size_changed = false;
	if (size.x > WorkSize.x) { size.x = WorkSize.x; size_changed = true; }
	if (size.y > WorkSize.y) { size.y = WorkSize.y; size_changed = true; }
	if (size_changed)
	{
		ImGui::SetWindowSize(size);
	}
	bool pos_changed = false;
	if (pos.x + size.x > WorkPos.x + WorkSize.x) { pos.x = WorkPos.x + WorkSize.x - size.x; pos_changed = true; }
	if (pos.y + size.y > WorkPos.y + WorkSize.y) { pos.y = WorkPos.y + WorkSize.y - size.y; pos_changed = true; }
	if (pos.x < WorkPos.x) { pos.x = WorkPos.x; pos_changed = true; }
	if (pos.y < WorkPos.y) { pos.y = WorkPos.y; pos_changed = true; }
	if (pos_changed)
	{
		ImGui::SetWindowPos(pos);
	}
}



/**
 * @brief 내부적으로 사용되는 윈도우 렌더링 처리
 * 서브클래스에서 직접 호출할 수 없도록 접근한정자 설정
 */
void UUIWindow::RenderWindow()
{
	// 숨겨진 상태면 렌더링하지 않음
	if (!IsVisible())
	{
		return;
	}

	// 도킹 설정 적용
	ApplyDockingSettings();

	// 메인 메뉴바 높이 고려용 오프셋 계산
	float MenuBarOffset = GetMenuBarOffset();

	// FIXME(KHJ): ImGui 윈도우 복원 로직이 잘 작동하지 않음. 중복 코드도 많아 개편 및 작동 수정 필요
	// ImGui 윈도우 시작
	// 복원이 필요한 경우 위치와 크기 강제 설정
	if (bShouldRestorePosition && RestoreFrameCount > 0)
	{
		ImVec2 AdjustedPosition = LastWindowPosition;
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		AdjustedPosition.y = max(AdjustedPosition.y, Viewport->WorkPos.y);
		ImGui::SetNextWindowPos(AdjustedPosition, ImGuiCond_Always);
		--RestoreFrameCount;
		if (RestoreFrameCount <= 0)
		{
			bShouldRestorePosition = false;
		}
	}

	else if (!bShouldRestorePosition)
	{
		ImVec2 AdjustedDefaultPosition = Config.DefaultPosition;
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		AdjustedDefaultPosition.y = max(AdjustedDefaultPosition.y, Viewport->WorkPos.y);
		ImGui::SetNextWindowPos(AdjustedDefaultPosition, ImGuiCond_FirstUseEver);
	}

	// ImGui의 내부 상태를 무시하고 강제로 적용
	if (bShouldRestoreSize && RestoreFrameCount > 0)
	{
		ImGui::SetNextWindowSize(LastWindowSize, ImGuiCond_Always);
		ImGui::SetNextWindowSizeConstraints(LastWindowSize, LastWindowSize);

		// ImGui 내부 상태 초기화를 위해 FirstUseEver도 시도
		ImGui::SetNextWindowSize(LastWindowSize, ImGuiCond_FirstUseEver);

		// 크기 복원도 같은 프레임 카운터 사용
		if (RestoreFrameCount <= 0)
		{
			bShouldRestoreSize = false;
			bForceSize = false;
			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(10000, 10000));
		}
	}

	else if (!bShouldRestoreSize)
	{
		ImGui::SetNextWindowSize(Config.DefaultSize, ImGuiCond_FirstUseEver);
	}

	bool bIsOpen = bIsWindowOpen;

	if (ImGui::Begin(Config.WindowTitle.data(), &bIsOpen, Config.WindowFlags))
	{
		// 잘 적용되지 않는 문제로 인해 여러 번 강제 적용 시도
		if (bShouldRestoreSize && RestoreFrameCount > 0)
		{
			ImGui::SetWindowSize(LastWindowSize, ImGuiCond_Always);
			ImGui::SetWindowSize(LastWindowSize);
			if (bShouldRestorePosition)
			{
				ImGui::SetWindowPos(LastWindowPosition, ImGuiCond_Always);
			}
		}

		if (!bIsResized)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 currentPos = ImGui::GetWindowPos();
			ImVec2 currentSize = ImGui::GetWindowSize();
			ImVec2 pivot = { 0.f, 0.f };

			PositionRatio.x = (currentPos.x - viewport->WorkPos.x + currentSize.x * pivot.x) / viewport->WorkSize.x;
			PositionRatio.y = (currentPos.y - viewport->WorkPos.y + currentSize.y * pivot.y) / viewport->WorkSize.y;

			SizeRatio.x = currentSize.x / viewport->WorkSize.x;
			SizeRatio.y = currentSize.y / viewport->WorkSize.y;
		}
		// 실제 UI 컨텐츠 렌더링
		RenderWidget();

		// 윈도우 정보 업데이트
		UpdateWindowInfo();
	}

	if (bIsResized)
	{
		OnMainWindowResized();
		bIsResized = false;
	}

	ImGui::End();

	// 윈도우가 닫혔는지 확인
	if (!bIsOpen && bIsWindowOpen)
	{
		// bClosable이 false인 경우
		if (!Config.bClosable)
		{
			// 열린 상태로 유지
			bIsWindowOpen = true;
			return;
		}

		// X 버튼이 클릭됨
		if (OnWindowClose())
		{
			bIsWindowOpen = false;
			SetWindowState(EUIWindowState::Hidden);

			const FString& Title = Config.WindowTitle;
			if (Title == "Outliner" || Title == "Details")
			{
				UUIManager::GetInstance().ForceArrangeRightPanels();
			}

			// 윈도우 숨김 이벤트 호출
			OnWindowHidden();
			UE_LOG("UIWindow: Window closed: %s", Config.WindowTitle.data());
		}
		else
		{
			// 닫기 취소
			bIsWindowOpen = true;
		}
	}
}

void UUIWindow::RenderWidget() const
{
	for (auto* Widget : Widgets)
	{
		Widget->RenderWidget();
		Widget->PostProcess(); // 선택
	}
}

void UUIWindow::Update() const
{
	for (UWidget* Widget : Widgets)
	{
		Widget->Update();
	}
}

/**
 * @brief 윈도우 위치와 크기 자동 설정
 */
void UUIWindow::ApplyDockingSettings() const
{
	ImGuiIO& IO = ImGui::GetIO();
	float ScreenWidth = IO.DisplaySize.x;
	float ScreenHeight = IO.DisplaySize.y;

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	float WorkPosY = Viewport ? Viewport->WorkPos.y : 0.0f;
	float WorkSizeY = Viewport ? Viewport->WorkSize.y : ScreenHeight;

	switch (Config.DockDirection)
	{
	case EUIDockDirection::Left:
		ImGui::SetNextWindowPos(ImVec2(0, WorkPosY), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(Config.DefaultSize.x, WorkSizeY), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Right:
		ImGui::SetNextWindowPos(ImVec2(ScreenWidth - Config.DefaultSize.x, WorkPosY), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(Config.DefaultSize.x, WorkSizeY), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Top:
		ImGui::SetNextWindowPos(ImVec2(0, WorkPosY), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, Config.DefaultSize.y), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Bottom:
		ImGui::SetNextWindowPos(ImVec2(0, WorkPosY + WorkSizeY - Config.DefaultSize.y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, Config.DefaultSize.y), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Center:
	{
		ImVec2 Center = ImVec2(ScreenWidth * 0.5f, WorkPosY + WorkSizeY * 0.5f);
		ImVec2 WindowPosition = ImVec2(Center.x - Config.DefaultSize.x * 0.5f,
			Center.y - Config.DefaultSize.y * 0.5f);
		// 최소 Y 위치는 작업 영역 시작 지점으로 제한
		WindowPosition.y = max(WindowPosition.y, WorkPosY);
		ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_FirstUseEver);
	}
	break;

	case EUIDockDirection::None:
	default:
		// 기본 위치 사용
		break;
	}
}

/**
 * @brief ImGui 컨텍스트에서 현재 윈도우 정보 업데이트
 */
void UUIWindow::UpdateWindowInfo()
{
	// 현재 윈도우가 포커스되어 있는지 확인
	bool bCurrentlyFocused = ImGui::IsWindowFocused();

	if (bCurrentlyFocused != bIsFocused)
	{
		bIsFocused = bCurrentlyFocused;

		if (bIsFocused)
		{
			OnFocusGained();
		}
		else
		{
			OnFocusLost();
		}
	}

	// 윈도우 크기와 위치 업데이트
	LastWindowSize = ImGui::GetWindowSize();
	LastWindowPosition = ImGui::GetWindowPos();
}


float UUIWindow::GetMenuBarOffset() const
{
	return UViewportManager::GetInstance().GetRoot()->GetRect().Y;
}
