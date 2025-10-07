#include "pch.h"
#include "Render/Viewport/Viewport.h"
#include "Manager/Input/InputManager.h"

// Intentionally minimal. All logic is inline in header for now.

FViewport::FViewport()
{
}

FViewport::~FViewport()
{
	SafeDelete(ViewportClient);
}


bool FViewport::HandleMouseDown(int32 InButton, int32 InLocalX, int32 InLocalY)
{
	LastMousePos = { InLocalX, InLocalY };
	// 캡처 시작 조건: 좌클릭(0)만 우선 지원
	if (InButton == 0)
	{
		bCapturing = true;
	}
	// ViewportClient의 명시적 버튼 콜백이 아직 없다면 여기서는 true만 반환
	return true;
}

bool FViewport::HandleMouseUp(int32 InButton, int32 InLocalX, int32 InLocalY)
{
	LastMousePos = { InLocalX, InLocalY };
	if (InButton == 0)
	{
		bCapturing = false;
	}
	return true;
}

bool FViewport::HandleMouseMove(int32 InLocalX, int32 InLocalY)
{
	LastMousePos = { InLocalX, InLocalY };
	if (ViewportClient)
	{
		ViewportClient->MouseMove(this, InLocalX, InLocalY);
		return true;
	}
	return false;
}

bool FViewport::HandleCapturedMouseMove(int32 InLocalX, int32 InLocalY)
{
	LastMousePos = { InLocalX, InLocalY };
	if (ViewportClient)
	{
		ViewportClient->CapturedMouseMove(this, InLocalX, InLocalY);
		return true;
	}
	return false;
}

void FViewport::PumpMouseFromInputManager()
{
	auto& Input = UInputManager::GetInstance();

	const FVector& MousePosWS = Input.GetMousePosition();
	const FPoint   Screen{ (LONG)MousePosWS.X, (LONG)MousePosWS.Y };

	// 로컬 좌표계 변환
	const FPoint Local = ToLocal(Screen);

	const LONG RenderAreaTop = Rect.Y + ToolBarLayPx;
	const LONG RenderAreaBottom = Rect.Y + Rect.H;
	const bool bInside = (Screen.X >= Rect.X) && (Screen.X < Rect.X + Rect.W) &&
		(Screen.Y >= RenderAreaTop) && (Screen.Y < RenderAreaBottom);

	// 버튼 상태 변화 감지 후 down/up 처리 (좌클릭 우선)
	if (Input.IsKeyPressed(EKeyInput::MouseLeft) && bInside)
	{
		HandleMouseDown(0, Local.X, Local.Y);
	}
	if (Input.IsKeyReleased(EKeyInput::MouseLeft))
	{
		HandleMouseUp(0, Local.X, Local.Y);
	}

	// 이동 라우팅: 캡처 중이면 Captured, 아니면 일반 Move
	const FVector& Delta = Input.GetMouseDelta();
	const bool bMoved = (Delta.X != 0.0f || Delta.Y != 0.0f);
	if (bMoved)
	{
		if (bCapturing)
		{
			HandleCapturedMouseMove(Local.X, Local.Y);
		}
		else if (bInside)
		{
			HandleMouseMove(Local.X, Local.Y);
		}
	}
}
