#pragma once
#include "Render/UI/Layout/Window.h"
//#include "Render/Viewport/FViewport.h"

// Leaf window that owns a FViewport
//class SViewportWindow : public SWindow
//{
//public:
//	explicit SViewportWindow(FViewportClient* InClient = nullptr)
//		: Viewport(InClient) {
//	}
//
//	void SetRect(const FRect& InRect) override
//	{
//		SWindow::SetRect(InRect);
//		Viewport.SetRect(InRect);
//	}
//
//	void Draw() override
//	{
//		Viewport.Present();
//	}
//
//	FViewport& GetViewport() { return Viewport; }
//
//private:
//	FViewport Viewport;
//};
