#pragma once

class UConsoleWindow;
class UControlPanelWindow;
class UExperimentalFeatureWindow;
class UPerformanceWindow;
class UOutlinerWindow;
class UDetailWindow;
class UCameraPanelWindow;
class UPIEWindow;
class UMainMenuWindow;
class ULevelTabBarWindow;

/**
 * @brief UI 윈도우 도킹 방향
 */
enum class EUIDockDirection : uint8_t
{
	None, // 도킹 없음
	Left, // 왼쪽 도킹
	Right, // 오른쪽 도킹
	Top, // 상단 도킹
	Bottom, // 하단 도킹
	Center, // 중앙 도킹
};

/**
* @brief UI 윈도우들을 쉽게 생성하기 위한 팩토리 클래스
 */
class UUIWindowFactory
{
public:
	static void CreateDefaultUILayout();

	// 맨위 메뉴바용 ui
	static UMainMenuWindow* CreateMainMenuWindow();

	static ULevelTabBarWindow* CreateLevelTabBarWindow();

	static UConsoleWindow* CreateConsoleWindow(EUIDockDirection InDockDirection = EUIDockDirection::Bottom);
	static UControlPanelWindow* CreateControlPanelWindow(EUIDockDirection InDockDirection = EUIDockDirection::Left);
	static UOutlinerWindow* CreateOutlinerWindow(EUIDockDirection InDockDirection = EUIDockDirection::Center);
	static UDetailWindow* CreateDetailWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);

	//static UPerformanceWindow* CreatePerformanceWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
	static UExperimentalFeatureWindow*
		CreateExperimentalFeatureWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
};
