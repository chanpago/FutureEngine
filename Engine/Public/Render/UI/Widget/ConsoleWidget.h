#pragma once
#include "Widget.h"

using std::streambuf;

class UConsoleWidget;
struct ImGuiInputTextCallbackData;

struct FLogEntry {
	ELogType Type;
	FString Message;
};

/**
 * @brief Custom Stream Buffer
 * Redirects Output to ConsoleWidget
 */
class ConsoleStreamBuffer : public streambuf
{
public:
	ConsoleStreamBuffer(UConsoleWidget* InConsole, bool bInIsError = false)
		: Console(InConsole), bIsError(bInIsError)
	{
	}

protected:
	// Stream Buffer 함수 Override
	int overflow(int InCharacter) override;
	streamsize xsputn(const char* InString, streamsize InCount) override;

private:
	UConsoleWidget* Console;
	FString Buffer;
	bool bIsError;
};

/**
 * @brief Console Widget
 * 콘솔의 실제 기능을 담당하는 위젯 (로그 표시, 명령어 처리 등)
 */
class UConsoleWidget : public UWidget
{
	DECLARE_CLASS(UConsoleWidget, UWidget)
public:
	UConsoleWidget();
	~UConsoleWidget();

	// Widget Interface
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Log Functions
	void AddLog(const char* fmt, ...);
	void AddLog(ELogType InType, const char* fmt, ...);
	void ClearLog();

	void ProcessCommand(const char* InCommand);
	void ExecuteTerminalCommand(const char* InCommand);

	void InitializeSystemRedirect();
	void CleanupSystemRedirect();
	void AddSystemLog(const char* InText, bool bInIsError = false);

	// History Navigation
	int HandleHistoryCallback(ImGuiInputTextCallbackData* InData);

private:
	// Helper Functions
	static ImVec4 GetColorByLogType(ELogType InType);

private:
	// Command Input
	char InputBuf[256];
	TArray<FString> CommandHistory;
	int HistoryPosition;

	// Log Output
	TArray<FLogEntry> LogItems;
	bool bIsAutoScroll;
	bool bIsScrollToBottom;

	// Stream Redirection
	ConsoleStreamBuffer* ConsoleOutputBuffer;
	ConsoleStreamBuffer* ConsoleErrorBuffer;
	streambuf* OriginalConsoleOutput;
	streambuf* OriginalConsoleError;
};
