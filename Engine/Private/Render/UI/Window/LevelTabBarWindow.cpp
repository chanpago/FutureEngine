#include "pch.h"
#include "public/Render/UI/Window/LevelTabBarWindow.h"
#include "public/Render/UI/Window/MainMenuWindow.h"
#include "public/Render/UI/Widget/LevelTabBarWidget.h"
#include "Public/Render/UI/Widget/ViewportControlWidget.h"

IMPLEMENT_SINGLETON(ULevelTabBarWindow)

ULevelTabBarWindow::ULevelTabBarWindow()
{
	SetupMainMenuConfig();
}
ULevelTabBarWindow::~ULevelTabBarWindow()
{

}


void ULevelTabBarWindow::Initialize()
{

	LevelTabBarWidget = new ULevelTabBarWidget;
	if (LevelTabBarWidget)
	{
		LevelTabBarWidget->Initialize();
		AddWidget(LevelTabBarWidget);
		UE_LOG("MainMenuWindow: MainBarWidget이 생성되고 초기화되었습니다");
	}
	else
	{
		UE_LOG("MainMenuWindow: Error: MainBarWidget 생성에 실패했습니다!");
		return;
	}

	UE_LOG("MainMenuWindow: 메인 메뉴 윈도우가 초기화되었습니다");

	WindowHeight = LevelTabBarWidget->GetLevelBarHeight();


	AddWidget(NewObject<UViewportControlWidget>());
}

void ULevelTabBarWindow::Cleanup()
{
	if (LevelTabBarWidget && !LevelTabBarWidget->IsSingleton())
	{
		SafeDelete(LevelTabBarWidget);
	}

	UUIWindow::Cleanup();
	UE_LOG("MainMenuWindow: 메인 메뉴 윈도우가 정리되었습니다");
}

void ULevelTabBarWindow::SetupMainMenuConfig()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "LevelTabBar";

	// 레벨 텝바 는 화면에 고정되어야 함
	Config.DefaultPosition = ImVec2(0, 56);
	Config.DefaultSize = ImVec2(0, 0);

	// 레벨 텝바 는 이동하거나 크기 조절할 수 없어야 함
	Config.bMovable = false;
	Config.bResizable = false;
	Config.bCollapsible = false;

	// 항상 표시되어야 함
	Config.InitialState = EUIWindowState::Visible;

	// 우선순위 설정
	// TODO(KHJ): Config 자체가 잘 작동하지 않는 부분이 있는데 이 우선순위는 의미가 없어보임. 개선 필요
	Config.Priority = 1000;

	// 탭만 보이도록 하기 위해 전체적으로 숨김 처리
	Config.WindowFlags = ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoDecoration;

	SetConfig(Config);
}
