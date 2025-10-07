#pragma once
#include "UIWindow.h"

class UWorld;

/**
 * @brief Outliner 역할을 제공할 Window
 * Scene에 존재하는 Actor들을 보여준다.
 */
class UOutlinerWindow : public UUIWindow
{
	DECLARE_CLASS(UOutlinerWindow, UUIWindow)
public:
    UOutlinerWindow();
	void Initialize() override;
};
