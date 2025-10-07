#include "pch.h"
#include "Render/UI/Window/OutlinerWindow.h"

#include "Render/UI/Widget/ActorListWidget.h"
#include "public/Manager/Viewport/ViewportManager.h"
#include "Render/UI/Layout/Window.h"

IMPLEMENT_CLASS(UOutlinerWindow, UUIWindow)

UOutlinerWindow::UOutlinerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Outliner";
	Config.DefaultSize = ImVec2(340, 520);
	Config.DefaultPosition = ImVec2(1565, 91); // 메뉴바만큼 하향 이동
	Config.MinSize = ImVec2(200, 50);
	Config.bResizable = true;
	Config.bMovable = false;
	Config.bCollapsible = false;
	Config.DockDirection = EUIDockDirection::Center;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(new UActorListWidget);
}

void UOutlinerWindow::Initialize()
{
	UE_LOG("OutlinerWindow: Successfully Initialized");
}
