#pragma once
#include "Render/UI/Window/UIWindow.h"
#include "public/Render/UI/Widget/MainBarWidget.h"

class UMainBarWidget;
class ULevelTabBarWidget;

class UMainMenuWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_SINGLETON(UMainMenuWindow)

public:
	void Initialize() override;
	void Cleanup() override;

	float GetMenuBarHeight() const { return WindowHeight; }
	UMainBarWidget* GetMainBarWidget() const { return MainBarWidget; }
	bool IsSingleton() override { return true; }

private:
	UMainBarWidget* MainBarWidget = nullptr;
	//ULevelTabBarWidget* LevelTabBarWidget = nullptr;



	void SetupMainMenuConfig();


};

