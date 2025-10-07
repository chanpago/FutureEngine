#include "pch.h"
#include "Core/AppWindow.h"
#include "resource.h"

#include "ImGui/imgui.h"
#include "Manager/UI/UIManager.h"
#include "Manager/Input/InputManager.h"
#include "Render/Renderer/Renderer.h"

FAppWindow::FAppWindow(FClientApp* InOwner)
	: Owner(InOwner), InstanceHandle(nullptr), MainWindowHandle(nullptr)
{
}

FAppWindow::~FAppWindow() = default;

bool FAppWindow::Init(HINSTANCE InInstance, int InCmdShow)
{
	InstanceHandle = InInstance;

	WCHAR WindowClass[] = L"UnlearnEngineWindowClass";

	// 아이콘 로드
	HICON hIcon = LoadIconW(InInstance, MAKEINTRESOURCEW(IDI_ICON1));
	HICON hIconSm = LoadIconW(InInstance, MAKEINTRESOURCEW(IDI_ICON1));
	WNDCLASSW wndclass = {};
	wndclass.style = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = InInstance;
	wndclass.hIcon = hIcon; // 큰 아이콘 (타이틀바용)
	wndclass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wndclass.hbrBackground = nullptr;
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = WindowClass;

	RegisterClassW(&wndclass);

	MainWindowHandle = CreateWindowExW(0, WindowClass, L"",
	                                   WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
	                                   CW_USEDEFAULT, CW_USEDEFAULT,
	                                   Render::INIT_SCREEN_WIDTH, Render::INIT_SCREEN_HEIGHT,
	                                   nullptr, nullptr, InInstance, nullptr);

	if (!MainWindowHandle)
	{
		return false;
	}

	if (hIcon)
	{
		SendMessageW(MainWindowHandle, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSm));
		SendMessageW(MainWindowHandle, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
	}

	ShowWindow(MainWindowHandle, InCmdShow);
	UpdateWindow(MainWindowHandle);
	SetNewTitle(L"Future Engine");

	return true;
}

/**
 * @brief Initialize Console & Redirect IO
 * 현재 ImGui로 기능을 넘기면서 사용은 하지 않으나 코드는 유지
 */
void FAppWindow::InitializeConsole()
{
	// Error Handle
	if (!AllocConsole())
	{
		MessageBoxW(nullptr, L"콘솔 생성 실패", L"Error", 0);
	}

	// Console 출력 지정
	FILE* FilePtr;
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stdout);
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stderr);
	(void)freopen_s(&FilePtr, "CONIN$", "r", stdin);

	// Console Menu Setting
	HWND ConsoleWindow = GetConsoleWindow();
	HMENU MenuHandle = GetSystemMenu(ConsoleWindow, FALSE);
	if (MenuHandle != nullptr)
	{
		EnableMenuItem(MenuHandle, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		DeleteMenu(MenuHandle, SC_CLOSE, MF_BYCOMMAND);
	}
}

FAppWindow* FAppWindow::GetWindowInstance(HWND InWindowHandle, uint32 InMessage, LPARAM InLParam)
{
	if (InMessage == WM_NCCREATE)
	{
		CREATESTRUCT* CreateStruct = reinterpret_cast<CREATESTRUCT*>(InLParam);
		FAppWindow* WindowInstance = static_cast<FAppWindow*>(CreateStruct->lpCreateParams);
		SetWindowLongPtr(InWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(WindowInstance));

		return WindowInstance;
	}

	return reinterpret_cast<FAppWindow*>(GetWindowLongPtr(InWindowHandle, GWLP_USERDATA));
}

LRESULT CALLBACK FAppWindow::WndProc(HWND InWindowHandle, uint32 InMessage, WPARAM InWParam,
                                     LPARAM InLParam)
{
	// Let ImGui process first, but do not swallow mouse messages; we'll decide in higher-level code
	UUIManager::WndProcHandler(InWindowHandle, InMessage, InWParam, InLParam);

	// Always forward to our input layer so editor can see mouse buttons even if ImGui wants capture
	UInputManager::GetInstance().ProcessKeyMessage(InMessage, InWParam, InLParam);


	RECT Rect{};

	switch (InMessage)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_ENTERSIZEMOVE: //드래그 시작
		URenderer::GetInstance().SetIsResizing(true);
		break;
	case WM_EXITSIZEMOVE: //드래그 종료
		URenderer::GetInstance().SetIsResizing(false);
		
		GetClientRect(InWindowHandle, &Rect);

		URenderer::GetInstance().OnResize((uint32)(Rect.right - Rect.left), (uint32)(Rect.bottom - Rect.top));
		UUIManager::GetInstance().RepositionImGuiWindows();
		break;
	case WM_SIZE:
		if (InWParam != SIZE_MINIMIZED)
		{
			if (!URenderer::GetInstance().GetIsResizing())
			{ //드래그 X 일때 추가 처리 (최대화 버튼, ...)
				URenderer::GetInstance().OnResize(LOWORD(InLParam), HIWORD(InLParam));
				UUIManager::GetInstance().RepositionImGuiWindows();
			}
		}
		break;

	default:
		return DefWindowProc(InWindowHandle, InMessage, InWParam, InLParam);
	}

	return 0;
}

void FAppWindow::SetNewTitle(const wstring& InNewTitle) const
{
	SetWindowTextW(MainWindowHandle, InNewTitle.c_str());
}

void FAppWindow::GetClientSize(int32& OutWidth, int32& OutHeight) const
{
	RECT ClientRectangle;
	GetClientRect(MainWindowHandle, &ClientRectangle);
	OutWidth = ClientRectangle.right - ClientRectangle.left;
	OutHeight = ClientRectangle.bottom - ClientRectangle.top;
}

FRect FAppWindow::GetClientSize() const
{
	RECT ClientRectangle;
	GetClientRect(MainWindowHandle, &ClientRectangle);

	FRect ClientSize{ ClientRectangle.left, ClientRectangle.top, ClientRectangle.right - ClientRectangle.left, ClientRectangle.top - ClientRectangle.bottom };

	return 	ClientSize;
	
}
