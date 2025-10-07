#include "pch.h"
#include "Render/UI/Window/ExperimentalFeatureWindow.h"

#include "Render/UI/Widget/InputInformationWidget.h"

IMPLEMENT_CLASS(UExperimentalFeatureWindow, UUIWindow)
/**
 * @brief Window Constructor
 */
UExperimentalFeatureWindow::UExperimentalFeatureWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Experimental Feature";
	Config.DefaultSize = ImVec2(400, 200);
	Config.DefaultPosition = ImVec2(10, 10); // 임시, 동적으로 계산됨
	Config.MinSize = ImVec2(350, 150);
	Config.DockDirection = EUIDockDirection::None;
	Config.Priority = 5;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(new UInputInformationWidget);
}

/**
 * @brief Initializer
 */
void UExperimentalFeatureWindow::Initialize()
{
	UE_LOG("ExperimentalFeatureWindow: Window가 성공적으로 생성되었습니다");
}
