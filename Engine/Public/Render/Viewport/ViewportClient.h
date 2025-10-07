#pragma once

#include "Editor/Camera.h"

class FViewport;

class FViewportClient
{
public:
	FViewportClient();
	~FViewportClient();

public:
	// ---------- 구성/질의 ----------
	void        SetViewType(EViewType InType) { ViewType = InType; }
	EViewType   GetViewType() const { return ViewType; }

	void        SetViewMode(EViewMode InMode) { ViewMode = InMode; }
	EViewMode   GetViewMode() const { return ViewMode; }


	bool        IsOrtho() const { return ViewType != EViewType::Perspective; }


public:
	void Tick() const;
	void Draw(const FViewport* InViewport) const;


	static void MouseMove(FViewport* /*Viewport*/, int32 /*X*/, int32 /*Y*/) {}
	void CapturedMouseMove(FViewport* /*Viewport*/, int32 X, int32 Y)
	{
		LastDrag = { X, Y };
	}

	// ---------- 리사이즈/포커스 ----------
	void OnResize(const FPoint& InNewSize) { ViewSize = InNewSize; }
	static void OnGainFocus() {}
	static void OnLoseFocus() {}
	static void OnClose() {}




	FViewProjConstants GetViewportCameraData() const { return ViewportCamera->GetFViewProjConstants(); }

	UCamera* GetCamera()const { return ViewportCamera; }

private:
	// 상태
	EViewType   ViewType = EViewType::Perspective;
	EViewMode   ViewMode = EViewMode::Lit;

	UCamera* ViewportCamera = nullptr;
	


	// 뷰/입력 상태
	FPoint		ViewSize{ 0, 0 };
	FPoint		LastDrag{ 0, 0 };
};
