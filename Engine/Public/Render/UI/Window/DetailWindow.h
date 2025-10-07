#pragma once
#include "UIWindow.h"

class UWorld;

/**
 * @brief Detail 역할을 제공할 Window
 * Actor Transform UI를 제공한다
 */
class UDetailWindow : public UUIWindow
{
	DECLARE_CLASS(UDetailWindow, UUIWindow)
public:
    UDetailWindow();
    void Initialize() override;
};
