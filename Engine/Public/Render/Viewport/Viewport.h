#pragma once
#include "Render/UI/Layout/Window.h"
#include "Render/Viewport/ViewportClient.h"

class FViewport
{
public:
	FViewport();
	virtual ~FViewport();

public:

	// ----- Client -----
	void SetViewportClient(FViewportClient* InClient) { ViewportClient = InClient; }
	FViewportClient* GetViewportClient() const { return ViewportClient; }

	// Geometry
	void SetSize(uint32 InSizeX, uint32 InSizeY) { { Rect.W = (LONG)InSizeX; Rect.H = (LONG)InSizeY; } }
	void SetInitialPosition(uint32 InPosX, uint32 InPosY) { { Rect.X = (LONG)InPosX; Rect.Y = (LONG)InPosY; } }
	void SetRect(const FRect& InRect) { Rect = InRect; }

	uint32       GetSizeX() const { return (uint32)max<LONG>(0, Rect.W); }
	uint32       GetSizeY() const { return (uint32)max<LONG>(0, Rect.H); }
	const FRect& GetRect()  const { return Rect; }

	// 보조
	float  GetAspect()  const { return (GetSizeY() > 0) ? float(GetSizeX()) / float(GetSizeY()) : 1.0f; }

	// 좌표 변환
	FORCEINLINE FPoint ToLocal(const FPoint& InScreen) const
	{
		return FPoint{ InScreen.X - Rect.X, InScreen.Y - Rect.Y };
	}
	FORCEINLINE FPoint ToScreen(const FPoint& InLocal) const
	{
		return FPoint{ InLocal.X + Rect.X, InLocal.Y + Rect.Y };
	}

	// ----- Input: 좌표는 "로컬(뷰) 공간" 기대 -----
	bool HandleMouseDown(int32 InButton, int32 InLocalX, int32 InLocalY);
	bool HandleMouseUp(int32 InButton, int32 InLocalX, int32 InLocalY);
	bool HandleMouseMove(int32 InLocalX, int32 InLocalY);
	bool HandleCapturedMouseMove(int32 InLocalX, int32 InLocalY);

	// InputManager로부터 현재 마우스 상태를 읽어 이 뷰포트 기준으로 라우팅
	// - 화면 좌표를 로컬로 변환하여 MouseMove/CapturedMouseMove, Down/Up을 전달
	void PumpMouseFromInputManager();

	// ----- D3D11 RS 바인딩 (옵션) -----
	void ApplyRasterizer(ID3D11DeviceContext* InDeviceContext) const
	{
		D3D11_VIEWPORT ViewPort{};
		ViewPort.TopLeftX = (FLOAT)Rect.X;
		ViewPort.TopLeftY = (FLOAT)Rect.Y + (FLOAT)ToolBarLayPx; // 툴바 제외하고 그릴 때
		ViewPort.Width = (FLOAT)max<LONG>(0, Rect.W);
		ViewPort.Height = (FLOAT)max<LONG>(0, Rect.H - ToolBarLayPx);
		ViewPort.MinDepth = 0.0f; ViewPort.MaxDepth = 1.0f;
		InDeviceContext->RSSetViewports(1, &ViewPort);

		D3D11_RECT Sc{ Rect.X, Rect.Y + ToolBarLayPx, Rect.X + Rect.W, Rect.Y + Rect.H };
		InDeviceContext->RSSetScissorRects(1, &Sc);
	}

	void SetToolbarHeight(int32 InPx) { ToolBarLayPx = max(0, InPx); }
	int32 GetToolbarHeight() const { return ToolBarLayPx; }

private:
	FViewportClient* ViewportClient = nullptr;
	FRect  Rect{ 0,0,0,0 };
	FPoint LastMousePos{ 0,0 };
	int32  ToolBarLayPx = 0;

	// 이 뷰포트가 현재 마우스 캡처(드래그) 중인지 여부
	bool   bCapturing = false;
};
