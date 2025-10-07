#include "pch.h"
#include "Render/UI/Window/DetailWindow.h"

#include "Render/UI/Widget/ActorDetailWidget.h"
#include "Render/UI/Widget/TargetComponentWidget.h"
#include "Render/UI/Widget/ActorTerminationWidget.h"

IMPLEMENT_CLASS(UDetailWindow, UUIWindow)

UDetailWindow::UDetailWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Details";
	Config.DefaultSize = ImVec2(350, 400);
	Config.DefaultPosition = ImVec2(1565, 590); // 임시, 동적으로 계산됨
	Config.MinSize = ImVec2(300, 300);
	Config.bResizable = true;
	Config.bMovable = false;
	Config.bCollapsible = false;
	Config.DockDirection = EUIDockDirection::Right;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(new UActorDetailWidget);
	AddWidget(new UActorTerminationWidget);
}

void UDetailWindow::Initialize()
{
	UE_LOG("DetailWindow: Successfully Initialized");
}
