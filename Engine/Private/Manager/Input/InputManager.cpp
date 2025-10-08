#include "pch.h"
#include "Manager/Input/InputManager.h"
#include "Core/AppWindow.h"
#include "public/Manager/Viewport/ViewportManager.h"
#include "Render/UI/Layout/Window.h"
#include "Public/Render/Viewport/Viewport.h"

IMPLEMENT_CLASS(UInputManager, UObject)
IMPLEMENT_SINGLETON(UInputManager)

UInputManager::UInputManager()
	: bIsWindowFocused(true), MouseWheelDelta(0.0f)
{
	InitializeKeyMapping();
}

UInputManager::~UInputManager() = default;

void UInputManager::InitializeKeyMapping()
{
	// 알파벳 키 매핑
	VirtualKeyMap['W'] = EKeyInput::W;
	VirtualKeyMap['A'] = EKeyInput::A;
	VirtualKeyMap['S'] = EKeyInput::S;
	VirtualKeyMap['D'] = EKeyInput::D;
	VirtualKeyMap['Q'] = EKeyInput::Q;
	VirtualKeyMap['E'] = EKeyInput::E;
	VirtualKeyMap['C'] = EKeyInput::C;

	// 화살표 키 매핑
	VirtualKeyMap[VK_UP] = EKeyInput::Up;
	VirtualKeyMap[VK_DOWN] = EKeyInput::Down;
	VirtualKeyMap[VK_LEFT] = EKeyInput::Left;
	VirtualKeyMap[VK_RIGHT] = EKeyInput::Right;

	// 액션 키 매핑
	VirtualKeyMap[VK_SPACE] = EKeyInput::Space;
	VirtualKeyMap[VK_RETURN] = EKeyInput::Enter;
	VirtualKeyMap[VK_ESCAPE] = EKeyInput::Esc;
	VirtualKeyMap[VK_TAB] = EKeyInput::Tab;
	VirtualKeyMap[VK_SHIFT] = EKeyInput::Shift;
	VirtualKeyMap[VK_CONTROL] = EKeyInput::Ctrl;
	VirtualKeyMap[VK_MENU] = EKeyInput::Alt;

	// 숫자 키 매핑
	VirtualKeyMap['0'] = EKeyInput::Num0;
	VirtualKeyMap['1'] = EKeyInput::Num1;
	VirtualKeyMap['2'] = EKeyInput::Num2;
	VirtualKeyMap['3'] = EKeyInput::Num3;
	VirtualKeyMap['4'] = EKeyInput::Num4;
	VirtualKeyMap['5'] = EKeyInput::Num5;
	VirtualKeyMap['6'] = EKeyInput::Num6;
	VirtualKeyMap['7'] = EKeyInput::Num7;
	VirtualKeyMap['8'] = EKeyInput::Num8;
	VirtualKeyMap['9'] = EKeyInput::Num9;

	// 마우스 버튼 (특별 처리 필요)
	VirtualKeyMap[VK_LBUTTON] = EKeyInput::MouseLeft;
	VirtualKeyMap[VK_RBUTTON] = EKeyInput::MouseRight;
	VirtualKeyMap[VK_MBUTTON] = EKeyInput::MouseMiddle;

	// 기타 키 매핑
	VirtualKeyMap[VK_F1] = EKeyInput::F1;
	VirtualKeyMap[VK_F2] = EKeyInput::F2;
	VirtualKeyMap[VK_F3] = EKeyInput::F3;
	VirtualKeyMap[VK_F4] = EKeyInput::F4;
	VirtualKeyMap[VK_BACK] = EKeyInput::Backspace;
	VirtualKeyMap[VK_DELETE] = EKeyInput::Delete;

	// 모든 키 상태를 false로 초기화
	for (int32 i = 0; i < static_cast<int32>(EKeyInput::End); ++i)
	{
		EKeyInput Key = static_cast<EKeyInput>(i);
		CurrentKeyState[Key] = false;
		PreviousKeyState[Key] = false;
	}
}

void UInputManager::Update(FAppWindow* InWindow)
{
	// 이전 프레임 상태를 현재 프레임 상태로 복사
	PreviousKeyState = CurrentKeyState;

	// 윈도우가 포커스를 잃었을 때는 입력 처리를 중단
	if (!bIsWindowFocused)
	{
		// 모든 키 상태를 false로 유지
		for (auto& Pair : CurrentKeyState)
		{
			Pair.second = false;
		}
		return;
	}

	// ImGui가 키보드를 잡고 있어도 전역 키(F1~F12 등)는 통과시킨다
	ImGuiIO& IO = ImGui::GetIO();
	bool bKeyboardCaptured = IO.WantCaptureKeyboard;

	// 마우스 위치 업데이트
	UpdateMousePosition(InWindow);

	// 마우스 휠 델타 리셋
	MouseWheelDelta = 0.0f;

	// GetAsyncKeyState를 사용하여 현재 키 상태를 업데이트
	for (auto& Pair : VirtualKeyMap)
	{
		int32 VirtualKey = Pair.first;
		EKeyInput KeyInput = Pair.second;

		// 마우스 버튼은 GetAsyncKeyState가 잘 작동하지 않을 수 있으므로 메시지 기반으로 처리
		if (KeyInput == EKeyInput::MouseLeft || KeyInput == EKeyInput::MouseRight || KeyInput ==
			EKeyInput::MouseMiddle)
		{
			// 마우스 버튼은 ProcessKeyMessage에서 처리
			continue;
		}

		// ImGui가 키보드를 캡쳐 중이면 전역키만 폴링 (F1~F12, Esc 등)
		bool bIsGlobalKey = (KeyInput == EKeyInput::F1 || KeyInput == EKeyInput::F2 || KeyInput == EKeyInput::F3 || KeyInput == EKeyInput::F4 || KeyInput == EKeyInput::Esc);
		if (bKeyboardCaptured && !bIsGlobalKey)
		{
			// 전역키가 아니면 상태 업데이트 스킵
			continue;
		}

		// GetAsyncKeyState의 반환값에서 최상위 비트가 1이면 키가 눌린 상태
		bool IsKeyDown = (GetAsyncKeyState(VirtualKey) & 0x8000) != 0;
		CurrentKeyState[KeyInput] = IsKeyDown;
	}
}

void UInputManager::UpdateMousePosition(const FAppWindow* InWindow)
{
	PreviousMousePosition = CurrentMousePosition;

	int32 ViewportWidth;
	int32 ViewportHeight;
	InWindow->GetClientSize(ViewportWidth, ViewportHeight);


	POINT MousePoint;
	if (GetCursorPos(&MousePoint))
	{
		ScreenToClient(GetActiveWindow(), &MousePoint);
		CurrentMousePosition.X = static_cast<float>(MousePoint.x);
		CurrentMousePosition.Y = static_cast<float>(MousePoint.y);
	}


	FRect vp;

	if (UViewportManager::GetInstance().GetViewportLayout() == EViewportLayout::Single)
	{
		vp = UViewportManager::GetInstance().GetRoot()->GetRect();
	}
	else if (UViewportManager::GetInstance().GetViewportLayout() == EViewportLayout::Quad)
	{
		int32 ViewportIndex = UViewportManager::GetInstance().GetActiveIndex();
		vp = UViewportManager::GetInstance().GetViewports()[ViewportIndex]->GetRect();
	}
	

	// 뷰포트 툴바 만큼 뷰포트를 내려주어야함. 지금 일단 브런치 사이클이 너무길어져서 머지를 미리 땡기고 추후 수정할것
	vp.Y += 32;
	vp.H -= 32;


	const float vw = std::max(1.0f, static_cast<float>(vp.W));
	const float vh = std::max(1.0f, static_cast<float>(vp.H));

	// 뷰포트 기준 로컬 좌표로 변환
	float localX = CurrentMousePosition.X - static_cast<float>(vp.X);
	float localY = CurrentMousePosition.Y - static_cast<float>(vp.Y);

	// (선택) 뷰포트 영역으로 클램프
	localX = std::clamp(localX, 0.0f, vw);
	localY = std::clamp(localY, 0.0f, vh);

	// NDC 변환
	NDCMousePosition.X = (localX / vw) * 2.0f - 1.0f;
	NDCMousePosition.Y = 1.0f - (localY / vh) * 2.0f;  // 위가 +1이 되도록 뒤집기

	MouseDelta = CurrentMousePosition - PreviousMousePosition;
}

bool UInputManager::IsKeyDown(EKeyInput InKey) const
{
	auto Iter = CurrentKeyState.find(InKey);
	if (Iter != CurrentKeyState.end())
	{
		return Iter->second;
	}
	return false;
}

bool UInputManager::IsKeyPressed(EKeyInput InKey) const
{
	auto CurrentIter = CurrentKeyState.find(InKey);
	auto PrevIter = PreviousKeyState.find(InKey);

	if (CurrentIter != CurrentKeyState.end() && PrevIter != PreviousKeyState.end())
	{
		// 이전 프레임에는 안 눌렸고, 현재 프레임에는 눌림
		return CurrentIter->second && !PrevIter->second;
	}

	return false;
}

bool UInputManager::IsKeyReleased(EKeyInput InKey) const
{
	auto CurrentIter = CurrentKeyState.find(InKey);
	auto PrevIter = PreviousKeyState.find(InKey);

	if (CurrentIter != CurrentKeyState.end() && PrevIter != PreviousKeyState.end())
	{
		// 이전 프레임에는 눌렸고, 현재 프레임에는 안 눌림
		return !CurrentIter->second && PrevIter->second;
	}
	return false;
}

void UInputManager::ProcessKeyMessage(uint32 InMessage, WPARAM WParam, LPARAM LParam)
{
	switch (InMessage)
	{
	//case WM_KEYDOWN:
	//case WM_SYSKEYDOWN:
	//	{
	//		auto it = VirtualKeyMap.find(static_cast<int32>(WParam));
	//		if (it != VirtualKeyMap.end())
	//		{
	//			CurrentKeyState[it->second] = true;
	//		}
	//	}
	//	break;

	//case WM_KEYUP:
	//case WM_SYSKEYUP:
	//	{
	//		auto it = VirtualKeyMap.find(static_cast<int32>(WParam));
	//		if (it != VirtualKeyMap.end())
	//		{
	//			CurrentKeyState[it->second] = false;
	//		}
	//	}
	//	break;

	case WM_LBUTTONDOWN:
		CurrentKeyState[EKeyInput::MouseLeft] = true;
		break;

	case WM_LBUTTONUP:
		CurrentKeyState[EKeyInput::MouseLeft] = false;
		break;

	case WM_RBUTTONDOWN:
		CurrentKeyState[EKeyInput::MouseRight] = true;
		break;

	case WM_RBUTTONUP:
		CurrentKeyState[EKeyInput::MouseRight] = false;
		break;

	case WM_MBUTTONDOWN:
		CurrentKeyState[EKeyInput::MouseMiddle] = true;
		break;

	case WM_MBUTTONUP:
		CurrentKeyState[EKeyInput::MouseMiddle] = false;
		break;

	case WM_MOUSEWHEEL:
		{
			// WParam의 상위 16비트에 휠 델타 값이 들어있음
			// WHEEL_DELTA(120)로 정규화하여 -1.0f ~ 1.0f 범위로 변환
			short WheelDelta = GET_WHEEL_DELTA_WPARAM(WParam);
			MouseWheelDelta = static_cast<float>(WheelDelta) / WHEEL_DELTA;
		}
		break;

	default:
		break;
	}
}

TArray<EKeyInput> UInputManager::GetKeysByStatus(EKeyStatus InStatus) const
{
	TArray<EKeyInput> Keys;

	for (int32 i = 0; i < static_cast<int32>(EKeyInput::End); ++i)
	{
		EKeyInput Key = static_cast<EKeyInput>(i);
		if (GetKeyStatus(Key) == InStatus)
		{
			Keys.push_back(Key);
		}
	}

	return Keys;
}

EKeyStatus UInputManager::GetKeyStatus(EKeyInput InKey) const
{
	auto CurrentIter = CurrentKeyState.find(InKey);
	auto PrevIter = PreviousKeyState.find(InKey);

	if (CurrentIter == CurrentKeyState.end() || PrevIter == PreviousKeyState.end())
	{
		return EKeyStatus::Unknown;
	}

	// Pressed -> Released -> Down -> Up
	if (CurrentIter->second && !PrevIter->second)
	{
		return EKeyStatus::Pressed;
	}
	if (!CurrentIter->second && PrevIter->second)
	{
		return EKeyStatus::Released;
	}
	if (CurrentIter->second)
	{
		return EKeyStatus::Down;
	}
	return EKeyStatus::Up;
}

TArray<EKeyInput> UInputManager::GetPressedKeys() const
{
	return GetKeysByStatus(EKeyStatus::Down);
}

TArray<EKeyInput> UInputManager::GetNewlyPressedKeys() const
{
	// Pressed 상태의 키들을 반환
	return GetKeysByStatus(EKeyStatus::Pressed);
}

TArray<EKeyInput> UInputManager::GetReleasedKeys() const
{
	// Released 상태의 키들을 반환
	return GetKeysByStatus(EKeyStatus::Released);
}

const wchar_t* UInputManager::KeyInputToString(EKeyInput InKey)
{
	switch (InKey)
	{
	case EKeyInput::W: return L"W";
	case EKeyInput::A: return L"A";
	case EKeyInput::S: return L"S";
	case EKeyInput::D: return L"D";
	case EKeyInput::Q: return L"Q";
	case EKeyInput::E: return L"E";
	case EKeyInput::C: return L"C";
	case EKeyInput::Space: return L"Space";
	case EKeyInput::Enter: return L"Enter";
	case EKeyInput::Esc: return L"Esc";
	case EKeyInput::Tab: return L"Tab";
	case EKeyInput::Shift: return L"Shift";
	case EKeyInput::Ctrl: return L"Ctrl";
	case EKeyInput::Alt: return L"Alt";
	case EKeyInput::Up: return L"Up";
	case EKeyInput::Down: return L"Down";
	case EKeyInput::Left: return L"Left";
	case EKeyInput::Right: return L"Right";
	case EKeyInput::MouseLeft: return L"MouseLeft";
	case EKeyInput::MouseRight: return L"MouseRight";
	case EKeyInput::MouseMiddle: return L"MouseMiddle";
	case EKeyInput::Num0: return L"Num0";
	case EKeyInput::Num1: return L"Num1";
	case EKeyInput::Num2: return L"Num2";
	case EKeyInput::Num3: return L"Num3";
	case EKeyInput::Num4: return L"Num4";
	case EKeyInput::Num5: return L"Num5";
	case EKeyInput::Num6: return L"Num6";
	case EKeyInput::Num7: return L"Num7";
	case EKeyInput::Num8: return L"Num8";
	case EKeyInput::Num9: return L"Num9";
	case EKeyInput::F1: return L"F1";
	case EKeyInput::F2: return L"F2";
	case EKeyInput::F3: return L"F3";
	case EKeyInput::F4: return L"F4";
	case EKeyInput::Backspace: return L"Backspace";
	case EKeyInput::Delete: return L"Delete";
	default: return L"Unknown";
	}
}

void UInputManager::SetWindowFocus(bool bInFocused)
{
	bIsWindowFocused = bInFocused;

	if (!bInFocused)
	{
		for (auto& Pair : CurrentKeyState)
		{
			Pair.second = false;
		}

		for (auto& Pair : PreviousKeyState)
		{
			Pair.second = false;
		}
	}
}
