#pragma once
#include "Render/UI/Window/UIWindow.h"


class ULevelTabBarWidget;

class ULevelTabBarWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_SINGLETON(ULevelTabBarWindow)

public:
	void Initialize() override;
	void Cleanup() override;

	ULevelTabBarWidget* GetMainBarWidget() const { return LevelTabBarWidget; }
	bool IsSingleton() override { return true; }

	float GetLevelBarHeight() const { return WindowHeight; }

private:
	ULevelTabBarWidget* LevelTabBarWidget = nullptr;



	void SetupMainMenuConfig();

};

