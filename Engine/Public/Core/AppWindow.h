#pragma once

class FClientApp;

/**
 * @brief 윈도우 생성 및 메시지 처리를 담당하는 클래스
 */
class FAppWindow
{
public:
	bool Init(HINSTANCE InInstance, int InCmdShow);
	static void InitializeConsole();

	// Viewport Information
	void GetClientSize(int32& OutWidth, int32& OutHeight) const;

	FRect GetClientSize()const;

	// Getter & Setter
	HWND GetWindowHandle() const { return MainWindowHandle; }
	HINSTANCE GetInstanceHandle() const { return InstanceHandle; }
	void SetNewTitle(const wstring& InNewTitle) const;

	// Special Member Function
	FAppWindow(FClientApp* InOwner);
	~FAppWindow();

private:
    FClientApp* Owner;
    HINSTANCE InstanceHandle;
    HWND MainWindowHandle;

    // Windows Callback Function
    static LRESULT CALLBACK WndProc(HWND InWindowHandle, uint32 InMessage, WPARAM InWParam, LPARAM InLParam);

    // Message Handle Function
    static FAppWindow* GetWindowInstance(HWND InWindowHandle, uint32 InMessage, LPARAM InLParam);
};
