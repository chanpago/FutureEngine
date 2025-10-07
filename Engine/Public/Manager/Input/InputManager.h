#pragma once
#include "Core/Object.h"

class FAppWindow;

class UInputManager : public UObject
{
	DECLARE_CLASS(UInputManager, UObject)
	DECLARE_SINGLETON(UInputManager)

public:
	void Update(FAppWindow* InWindow);
	void UpdateMousePosition(const FAppWindow* InWindow);
	void ProcessKeyMessage(uint32 InMessage, WPARAM WParam, LPARAM LParam);

	bool IsKeyDown(EKeyInput InKey) const;
	bool IsKeyPressed(EKeyInput InKey) const;
	bool IsKeyReleased(EKeyInput InKey) const;

	// Key Status
	EKeyStatus GetKeyStatus(EKeyInput InKey) const;
	TArray<EKeyInput> GetKeysByStatus(EKeyStatus InStatus) const;

	TArray<EKeyInput> GetPressedKeys() const;
	TArray<EKeyInput> GetNewlyPressedKeys() const;
	TArray<EKeyInput> GetReleasedKeys() const;

	// Window Focus Management
	void SetWindowFocus(bool bInFocused);
	bool IsWindowFocused() const { return bIsWindowFocused; }

	// Helper Function
	static const wchar_t* KeyInputToString(EKeyInput InKey);

	// Mouse Wheel
	float GetMouseWheelDelta() const { return MouseWheelDelta; }

	// Getter
	const FVector& GetMouseNDCPosition() const { return NDCMousePosition; }
	const FVector& GetMousePosition() const { return CurrentMousePosition; }
	const FVector& GetMouseDelta() const { return MouseDelta; }

private:
	// Key Status
	TMap<EKeyInput, bool> CurrentKeyState;
	TMap<EKeyInput, bool> PreviousKeyState;
	TMap<int32, EKeyInput> VirtualKeyMap;

	// Mouse Position
	FVector CurrentMousePosition;
	FVector PreviousMousePosition;
	FVector MouseDelta;

	// Mouse Wheel
	float MouseWheelDelta;

	// NDC Mouse Position
	FVector NDCMousePosition;

	// Window Focus
	bool bIsWindowFocused;

	void InitializeKeyMapping();
};
