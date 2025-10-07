#pragma once

class UEditor;
class FAppWindow;

/**
 * @brief Main Client Class
 * Application Entry Point, Manage Overall Execution Flow
 *
 * @var AcceleratorTable 키보드 단축키 테이블 핸들
 * @var MainMessage 윈도우 메시지 구조체
 * @var Window 윈도우 객체 포인터
 */
class FClientApp
{
public:
    int Run(HINSTANCE InInstanceHandle, int InCmdShow);

    // Special Member Function
    FClientApp();
    ~FClientApp();

private:
	// 로그/크래시/플랫폼/설정/모듈/경로/메모리 등 코어 준비
	void PreInit(HINSTANCE InInstanceHandle, int InCmdShow);

	// RHI/Slate/오디오/네트워킹, GEngine/World 생성, (에디터면 UEditorEngine)
	void Init();

	// 메시지 펌프, 입력, Slate, UWorld/Actors Tick, GC, 렌더 커맨드 큐, Present
	void Tick();

	// 서브시스템/모듈/리소스 정리, RHI/Slate 종료
	void Shutdown();




    int InitializeSystem();
    void UpdateSystem();
    void MainLoop();
	void ShutdownSystem();
private:
	HACCEL AcceleratorTable{};
	MSG MainMessage{};
    FAppWindow* Window = nullptr;
	UEditor* Editor = nullptr;
};
