#pragma once
#include "Core/Object.h"

class UUIWindow;
class UImGuiHelper;
class AActor;
class USceneComponent;
class UMainMenuWindow;
class ULevelBarWindow;

/**
 * @brief UI 매니저 클래스
 * 모든 UI 윈도우를 관리하는 싱글톤 클래스
 * @param UIWindows 등록된 모든 UI 윈도우들
 * @param FocusedWindow 현재 포커스된 윈도우
 * @param bIsInitialized 초기화 상태
 * @param TotalTime 전체 경과 시간
 */
class UUIManager : public UObject
{
	DECLARE_CLASS(UUIManager, UObject)
	DECLARE_SINGLETON(UUIManager)

public:
	void Initialize();
	void Initialize(HWND InWindowHandle);
	void Release();
	void Update();
	void Render();
	bool RegisterUIWindow(UUIWindow* InWindow);
	bool UnregisterUIWindow(UUIWindow* InWindow);
	void PrintDebugInfo() const;

	UUIWindow* FindUIWindow(const FString& InWindowName) const;
	UWidget* FindWidget(const FString& InWidgetName) const;
	void HideAllWindows() const;
	void ShowAllWindows() const;

	// 윈도우 최소화 / 복원 처리
	void OnWindowMinimized();
	void OnWindowRestored();


	// Getter & Setter
	size_t GetUIWindowCount() const { return UIWindows.size(); }
	const TArray<UUIWindow*>& GetAllUIWindows() const { return UIWindows; }
	UUIWindow* GetFocusedWindow() const { return FocusedWindow; }
	USceneComponent* GetSelectedComponent() const { return SelectedComponent; }
	AActor* GetSelectedActor() const { return SelectedActor; }


	void SetSelectedActor(AActor* InActor);
	void SetSelectedComponent(USceneComponent* InComponent) { SelectedComponent = InComponent; }
	void SetFocusedWindow(UUIWindow* InWindow);

	// ImGui 관련 메서드
	static LRESULT WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

	void RepositionImGuiWindows();

	// 메인 메뉴바 관련 메서드
	void RegisterMainMenuWindow(UMainMenuWindow* InMainMenuWindow);
	float GetMainMenuBarHeight() const;
	float GetRightPanelWidth() const;
	void ArrangeRightPanels();
	void ForceArrangeRightPanels();
	void OnPanelVisibilityChanged();


	// 레벨 바 관련 메서드
	void RegisterLevelTabBarWindow(ULevelTabBarWindow* InLevelBarWindow);

	// F1 키로 Experimental Feature Window 토글
	//void ToggleExperimentalFeatureWindow();



private:
	void SortUIWindowsByPriority();
	void UpdateFocusState();

private:
	AActor* SelectedActor = nullptr;
	USceneComponent* SelectedComponent = nullptr;
	TArray<UUIWindow*> UIWindows;
	UUIWindow* FocusedWindow = nullptr;
	bool bIsInitialized = false;
	float TotalTime = 0.0f;

	// 윈도우 상태 저장
	struct FUIWindowSavedState
	{
		uint32 WindowID;
		ImVec2 SavedPosition;
		ImVec2 SavedSize;
		bool bWasVisible;
	};

	TArray<FUIWindowSavedState> SavedWindowStates;
	bool bIsMinimized = false;

	// Main Menu Window
	UMainMenuWindow* MainMenuWindow = nullptr;

	// 레벨바 윈도우
	ULevelTabBarWindow* LevelTabBarWindow = nullptr;

	// Experimental Feature Window 토글용
	class UExperimentalFeatureWindow* ExperimentalWindow = nullptr;

	// ImGui Helper
	UImGuiHelper* ImGuiHelper = nullptr;

	// 오른쪽 패널 상태 추적 변수들
	float SavedOutlinerHeightForDual = 0.0f; // 두 패널이 있을 때 Outliner 높이
	float SavedDetailHeightForDual = 0.0f; // 두 패널이 있을 때 Detail 높이
	float SavedPanelWidth = 0.0f; // 기억된 패널 너비
	bool bHasSavedDualLayout = false; // 두 패널 레이아웃이 저장되어 있는지



	// 오른쪽 패널 레이아웃 헬퍼 함수
	void ArrangeRightPanelsInitial(UUIWindow* InOutlinerWindow, UUIWindow* InDetailWindow,
		float InScreenWidth, float InScreenHeight, float InMenuBarHeight,
		float InAvailableHeight);
	void ArrangeRightPanelsDynamic(UUIWindow* InOutlinerWindow, UUIWindow* InDetailWindow,
		float InScreenWidth, float InScreenHeight, float InMenuBarHeight,
		float InAvailableHeight, float InTargetWidth) const;

};
