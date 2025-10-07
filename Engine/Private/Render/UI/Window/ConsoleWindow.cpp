#include "pch.h"
#include "Render/UI/Window/ConsoleWindow.h"
#include "Render/UI/Widget/ConsoleWidget.h"

IMPLEMENT_SINGLETON(UConsoleWindow)
IMPLEMENT_CLASS(UConsoleWindow, UUIWindow)

UConsoleWindow::~UConsoleWindow()
{
	if (ConsoleWidget)
	{
		ConsoleWidget->CleanupSystemRedirect();
		ConsoleWidget->ClearLog();
	}
}

UConsoleWindow::UConsoleWindow()
{
	// 콘솔 윈도우 기본 설정
	FUIWindowConfig Config;
	Config.WindowTitle = "Future Console";
	Config.DefaultSize = ImVec2(800, 200);
	Config.DefaultPosition = ImVec2(10, 10); // 임시, 동적으로 계산됨
	Config.MinSize = ImVec2(600, 150);
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::None;
	SetConfig(Config);

	ConsoleWidget = new UConsoleWidget;
	AddWidget(ConsoleWidget);
}

void UConsoleWindow::Initialize()
{
	// Initialize System Output Redirection
	ConsoleWidget->InitializeSystemRedirect();

	AddLog(ELogType::Success, "ConsoleWindow: Game Console 초기화 성공");
	AddLog(ELogType::System, "ConsoleWindow: Logging System Ready");
}

/**
 * @brief 외부에서 ConsoleWidget에 접근할 수 있도록 하는 래핑 함수
 */
void UConsoleWindow::AddLog(const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(buf);
	}
}

void UConsoleWindow::AddLog(ELogType InType, const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(InType, buf);
	}
}

void UConsoleWindow::AddSystemLog(const char* InText, bool bInIsError) const
{
	if (ConsoleWidget)
	{
		ConsoleWidget->AddSystemLog(InText, bInIsError);
	}
}
