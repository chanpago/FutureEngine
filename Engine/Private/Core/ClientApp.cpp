#include "pch.h"
#include "Core/ClientApp.h"

#include "Editor/Editor.h"
#include "Core/AppWindow.h"
#include "Manager/Input/InputManager.h"
#include "Manager/Level/World.h"
#include "Manager/Time/TimeManager.h"
#include "Manager/Viewport/ViewportManager.h"

#include "Manager/UI/UIManager.h"
#include "Render/Renderer/Renderer.h"
#include "Manager/Overlay/OverlayManager.h"

#include "Render/UI/Window/ConsoleWindow.h"

#include "Editor/EditorEngine.h"

#include <chrono> 

#pragma comment(lib, "winmm.lib")


FClientApp::FClientApp() = default;

FClientApp::~FClientApp() = default;
void FClientApp::PreInit(HINSTANCE InInstanceHandle, int InCmdShow)
{
	// Memory Leak Detection & Report
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(0);
#endif

	// Window Object Initialize
	Window = new FAppWindow(this);
	if (!Window->Init(InInstanceHandle, InCmdShow))
	{
		assert(!"Window Creation Failed");
	}

}
void FClientApp::Init()
{
	// Initialize Base System
	int InitResult = InitializeSystem();
	if (InitResult != S_OK)
	{
		assert(!"Initialize Failed");
	}

	bool bInitResult = InitEditorEngine();
	if (!bInitResult)
	{
		assert("Editor Endgine Initialize Failed");
	}
}
void FClientApp::Tick()
{
	auto& TimeManager = UTimeManager::GetInstance();
	auto& InputManager = UInputManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();
	//auto& LevelManager = GWorld;
	auto& UiManager = UUIManager::GetInstance();
	auto& OverlayManager = UOverlayManager::GetInstance();
	auto& ViewportManger = UViewportManager::GetInstance();


	TimeManager.Update();

	Editor->Update();

	GEditor->Tick();

	InputManager.Update(Window);
	UiManager.Update();


	ViewportManger.Update();

	OverlayManager.UpdateFPS();


	Renderer.Update(Editor);
}
void FClientApp::Shutdown()
{
	// Editor shutdown with settings save
	if (Editor)
	{
		Editor->Shutdown();
	}

	if (GEditor)
	{
		ShutdownEditorEngine();
	}

	UUIManager::GetInstance().Release();
	URenderer::GetInstance().Release();
	GWorld->Release();
	UResourceManager::GetInstance().Release();
	UOverlayManager::GetInstance().Release();

	// 레벨 매니저 정리
	// GWorld->Release();
	delete Editor;
	delete Window;
	delete GWorld;
}
/**
 * @brief Client Main Runtime Function
 * App 초기화, Main Loop 실행을 통한 전체 Cycle
 *
 * @param InInstanceHandle Process Instance Handle
 * @param InCmdShow Window Display Method
 *
 *
 * @return Program Termination Code
 */
int FClientApp::Run(HINSTANCE InInstanceHandle, int InCmdShow)
{
	// 로그/크래시/플랫폼/설정/모듈/경로/메모리 등 코어 준비
	PreInit(InInstanceHandle, InCmdShow);

	// Initialize base system
	Init();

	// Execute main loop
	MainLoop();

	// Shutdown system
	Shutdown();

	return static_cast<int>(MainMessage.wParam);
}

/**
 * @brief Initialize System For Game Execution
 */
int FClientApp::InitializeSystem()
{
	// Initialize By Get Instance
	UTimeManager::GetInstance();
	UInputManager::GetInstance();

	// Renderer Initialize
	auto& Renderer = URenderer::GetInstance();
	Renderer.Init(Window->GetWindowHandle());

	// UIManager Initialize
	auto& UiManager = UUIManager::GetInstance();
	UiManager.Initialize(Window->GetWindowHandle());
	UUIWindowFactory::CreateDefaultUILayout();

	// ResourceManager Initialize
	UResourceManager::GetInstance().Initialize();


	UViewportManager::GetInstance().Initialize(Window);

	// Initialize Editor
	Editor = NewObject<UEditor>();
	Editor->Initialize();

	return S_OK;
}

/**
 * @brief Update System While Game Processing
 */
void FClientApp::UpdateSystem()
{
	auto& TimeManager = UTimeManager::GetInstance();
	auto& InputManager = UInputManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();
	//auto& LevelManager = GWorld;
	auto& UiManager = UUIManager::GetInstance();
	auto& OverlayManager = UOverlayManager::GetInstance();
	auto& ViewportManager = UViewportManager::GetInstance();
	TimeManager.Update();
	Editor->Update();
	 
	GEditor->Tick();

	InputManager.Update(Window);
	UiManager.Update();	   

	OverlayManager.UpdateFPS();
	Renderer.Update(Editor);
	
	//OverlayManager.RenderOverlay();
}

/**
 * @brief Execute Main Message Loop
 * 윈도우 메시지 처리 및 게임 시스템 업데이트를 담당
 * 60fps로 프레임 제한
 */
void FClientApp::MainLoop()
{
	// 고정밀 타이머 설정 (1ms 해상도)
	timeBeginPeriod(1);
	
	const double TargetFPS = 60.0;
	const double TargetFrameTime = 1000.0 / TargetFPS; // 16.666... ms
	
	LARGE_INTEGER Frequency, LastTime, CurrentTime;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastTime);

	while (true)
	{ 
		// Process all pending messages
		while (PeekMessage(&MainMessage, nullptr, 0, 0, PM_REMOVE))
		{
			// Process Termination
			if (MainMessage.message == WM_QUIT)
			{
				timeEndPeriod(1); // 타이머 해상도 복구
				return;
			}
			
			// Shortcut Key Processing
			if (!TranslateAccelerator(MainMessage.hwnd, AcceleratorTable, &MainMessage))
			{
				TranslateMessage(&MainMessage);
				DispatchMessage(&MainMessage);
			}
		}
		Tick();
	}
}

/**
 * @brief 시스템 종료 처리
 * 모든 리소스를 안전하게 해제하고 매니저들을 정리합니다.
 */
void FClientApp::ShutdownSystem()
{
	// Editor shutdown with settings save
	if (Editor)
	{
		Editor->Shutdown();
	}

	if (GEditor)
	{
		ShutdownEditorEngine();
	}

	UUIManager::GetInstance().Release();
	URenderer::GetInstance().Release();
	GWorld->Release();
	UResourceManager::GetInstance().Release();
	UOverlayManager::GetInstance().Release();

	// 레벨 매니저 정리
	// GWorld->Release();
	delete Editor;
	delete Window;
	delete GWorld;
}
