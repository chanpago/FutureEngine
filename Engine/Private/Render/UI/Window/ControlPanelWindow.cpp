#include "pch.h"
#include "Render/UI/Window/ControlPanelWindow.h"

#include "Render/UI/Widget/CameraControlWidget.h"
#include "Render/UI/Widget/FPSWidget.h"
#include "Render/UI/Widget/PrimitiveSpawnWidget.h"
#include "Render/UI/Widget/SceneIOWidget.h"
#include "Render/UI/Widget/ViewSettingsWidget.h"

IMPLEMENT_CLASS(UControlPanelWindow, UUIWindow)

/**
 * @brief Control Panel Constructor
 * 적절한 사이즈의 윈도우 제공
 */
UControlPanelWindow::UControlPanelWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Control Panel";
	Config.DefaultSize = ImVec2(400, 620);
	Config.DefaultPosition = ImVec2(10, 10);
	Config.MinSize = ImVec2(350, 200);
	Config.DockDirection = EUIDockDirection::None;
	Config.Priority = 15;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(NewObject<UFPSWidget>());
	AddWidget(NewObject<UPrimitiveSpawnWidget>());
	AddWidget(NewObject<USceneIOWidget>());
	AddWidget(NewObject<UCameraControlWidget>());
	AddWidget(NewObject<UViewSettingsWidget>());
}

/**
 * @brief 초기화 함수
 */
void UControlPanelWindow::Initialize()
{
	UE_LOG("ControlPanelWindow: Initialized");
}
