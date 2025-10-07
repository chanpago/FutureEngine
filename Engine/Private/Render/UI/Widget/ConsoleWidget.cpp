#include "pch.h"
#include "Render/UI/Widget/ConsoleWidget.h"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <vector>
#include <stdexcept>

IMPLEMENT_CLASS(UConsoleWidget, UWidget)

UConsoleWidget::UConsoleWidget() = default;

UConsoleWidget::~UConsoleWidget()
{
	CleanupSystemRedirect();
	ClearLog();
}

void UConsoleWidget::Initialize()
{
	// 초기화
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPosition = -1;
	bIsAutoScroll = true;
	bIsScrollToBottom = false;

	// Stream Redirection 초기화
	ConsoleOutputBuffer = nullptr;
	ConsoleErrorBuffer = nullptr;
	OriginalConsoleOutput = nullptr;
	OriginalConsoleError = nullptr;

	AddLog(ELogType::Success, "ConsoleWindow: Game Console 초기화 성공");
	AddLog(ELogType::System, "ConsoleWindow: Logging System Ready");

	// Initialize System Output Redirection
	InitializeSystemRedirect();
}

/**
 * @brief StreamBuffer Override 함수
 * @param InCharacter classic type char
 * @return 입력 받은 문자
 */
int ConsoleStreamBuffer::overflow(int InCharacter)
{
	if (InCharacter != EOF)
	{
		Buffer += static_cast<char>(InCharacter);
		if (InCharacter == '\n')
		{
			if (Console)
			{
				Console->AddSystemLog(Buffer.c_str(), bIsError);
			}
			Buffer.clear();
		}
	}
	return InCharacter;
}

streamsize ConsoleStreamBuffer::xsputn(const char* InString, streamsize InCount)
{
	for (std::streamsize i = 0; i < InCount; ++i)
	{
		overflow(InString[i]);
	}
	return InCount;
}


void UConsoleWidget::RenderWidget()
{
	// 제어 버튼들
	if (ImGui::Button("Clear"))
	{
		ClearLog();
	}

	ImGui::SameLine();
	if (ImGui::Button("Copy"))
	{
		ImGui::LogToClipboard();
	}

	// ImGui::SameLine();
	// ImGui::Checkbox("AutoScroll", &bIsAutoScroll);

	ImGui::Separator();

	// 로그 출력 영역 미리 예약
	const float ReservedHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("LogOutput", ImVec2(0, -ReservedHeight), ImGuiChildFlags_NavFlattened,
	                      ImGuiWindowFlags_HorizontalScrollbar))
	{
		// 로그 리스트 출력
		for (const auto& LogEntry : LogItems)
		{
			// ELogType을 기반으로 색상 결정
			ImVec4 Color = GetColorByLogType(LogEntry.Type);
			bool bShouldApplyColor = (LogEntry.Type != ELogType::Info);

			if (bShouldApplyColor)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Color);
			}

			ImGui::TextUnformatted(LogEntry.Message.c_str());

			if (bShouldApplyColor)
			{
				ImGui::PopStyleColor();
			}
		}

		// Auto Scroll
		if (bIsScrollToBottom || (bIsAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		{
			ImGui::SetScrollHereY(1.0f);
		}
		bIsScrollToBottom = false;
	}

	ImGui::EndChild();
	ImGui::Separator();

	// Input Command with History Navigation
	bool ReclaimFocus = false;
	ImGuiInputTextFlags InputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
		ImGuiInputTextFlags_CallbackHistory;
	if (ImGui::InputText("Input", InputBuf, sizeof(InputBuf), InputFlags,
	                     [](ImGuiInputTextCallbackData* data) -> int
	                     {
		                     return static_cast<UConsoleWidget*>(data->UserData)->HandleHistoryCallback(data);
	                     }, this))
	{
		if (strlen(InputBuf) > 0)
		{
			ProcessCommand(InputBuf);
			strcpy_s(InputBuf, sizeof(InputBuf), "");
			ReclaimFocus = true;
			// 새로운 명령어 입력 후 히스토리 위치 초기화
			HistoryPosition = -1;
		}
	}

	// 입력 포커스 설정
	ImGui::SetItemDefaultFocus();
	if (ReclaimFocus)
	{
		ImGui::SetKeyboardFocusHere(-1);
	}
}

void UConsoleWidget::Update()
{
	// 필요한 경우 여기에 업데이트 로직 추가할 것
	// ImGui 위주라서 필요하지 않을 것으로 보이긴 함...
}

void UConsoleWidget::ClearLog()
{
	LogItems.clear();
}

/**
 * @brief ELogType에 따른 색상 반환
 */
ImVec4 UConsoleWidget::GetColorByLogType(ELogType InType)
{
	switch (InType)
	{
	case ELogType::Info:
		return {1.0f, 1.0f, 1.0f, 1.0f}; // 흰색
	case ELogType::Warning:
		return {1.0f, 1.0f, 0.4f, 1.0f}; // 노란색
	case ELogType::Error:
	case ELogType::TerminalError:
		return {1.0f, 0.4f, 0.4f, 1.0f}; // 빨간색
	case ELogType::Success:
	case ELogType::UELog:
		return {0.4f, 1.0f, 0.4f, 1.0f}; // 초록색
	case ELogType::System:
		return {0.7f, 0.7f, 0.7f, 1.0f}; // 회색
	case ELogType::Debug:
		return {0.4f, 0.6f, 1.0f, 1.0f}; // 파란색
	case ELogType::Terminal:
		return {0.6f, 0.8f, 1.0f, 1.0f}; // 하늘색
	case ELogType::Command:
		return {1.0f, 0.8f, 0.6f, 1.0f}; // 주황색
	default:
		return {1.0f, 1.0f, 1.0f, 1.0f}; // 기본 흰색
	}
}

/**
 * @brief 기본 로그 추가 함수
 */
void UConsoleWidget::AddLog(const char* fmt, ...)
{
	char buf[1024];

	// Variable Argument List 접근
	va_list args;
	va_start(args, fmt);

	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = 0;

	va_end(args);

	// 기본 Info 타입으로 로그 추가
	FLogEntry LogEntry;
	LogEntry.Type = ELogType::Info;
	LogEntry.Message = FString(buf);
	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}

/**
 * @brief 타입 특정 로그 추가 함수
 */
void UConsoleWidget::AddLog(ELogType InType, const char* fmt, ...)
{
	char buf[1024];

	// Variable Argument List 접근
	va_list args;
	va_start(args, fmt);

	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = 0;

	va_end(args);

	// 지정된 타입으로 로그 추가
	FLogEntry LogEntry;
	LogEntry.Type = InType;
	LogEntry.Message = FString(buf);
	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}

/**
 * @brief 명령어 히스토리 탐색 콜백 함수
 * @param InData ImGui InputText 콜백 데이터
 * @return 콜백 처리 결과 (0: 성공)
 */
int UConsoleWidget::HandleHistoryCallback(ImGuiInputTextCallbackData* InData)
{
	switch (InData->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackHistory:
		{
			// 비어있는 히스토리는 처리하지 않음
			if (CommandHistory.empty())
			{
				return 0;
			}

			const int HistorySize = static_cast<int>(CommandHistory.size());
			int PreviousHistoryPos = HistoryPosition;

			if (InData->EventKey == ImGuiKey_UpArrow)
			{
				// 위 화살표: 이전 명령어로 이동
				if (HistoryPosition == -1)
				{
					// 처음에는 가장 최근 명령어로 이동
					HistoryPosition = HistorySize - 1;
				}
				else if (HistoryPosition > 0)
				{
					// 더 이전 명령어로 이동
					--HistoryPosition;
				}
			}
			else if (InData->EventKey == ImGuiKey_DownArrow)
			{
				// 아래 화살표: 다음 명령어로 이동
				if (HistoryPosition != -1)
				{
					++HistoryPosition;
					if (HistoryPosition >= HistorySize)
					{
						// 범위를 벗어나면 입력창을 비우고 현재 상태로 돌아감
						HistoryPosition = -1;
					}
				}
			}

			// 히스토리 위치가 변경되었을 때만 입력창 업데이트
			if (PreviousHistoryPos != HistoryPosition)
			{
				if (HistoryPosition >= 0 && HistoryPosition < HistorySize)
				{
					// 선택된 히스토리 명령어로 입력창 채우기
					const FString& SelectedCommand = CommandHistory[HistoryPosition];
					InData->DeleteChars(0, InData->BufTextLen);
					InData->InsertChars(0, SelectedCommand.c_str());
				}
				else
				{
					// HistoryPosition == -1: 입력창 비우기
					InData->DeleteChars(0, InData->BufTextLen);
				}
			}
		}
		break;

	default:
		break;
	}

	return 0;
}

void UConsoleWidget::ProcessCommand(const char* InCommand)
{
	if (!InCommand || strlen(InCommand) == 0)
	{
		return;
	}

	// 명령 히스토리에 추가
	CommandHistory.push_back(FString(InCommand));
	HistoryPosition = -1;

	FString Input = InCommand;

	// UE_LOG( 우선 탐색
	size_t StartPosition = Input.find("UE_LOG(");
	if (StartPosition != FString::npos && StartPosition == 0)
	{
		StartPosition += 7; // "UE_LOG(" 길이
		size_t EndPosition = Input.rfind(')');

		if (EndPosition != FString::npos && EndPosition > StartPosition)
		{
			FString Arguments = Input.substr(StartPosition, EndPosition - StartPosition);

			// 첫 번째 인수는 문자열 (" 로 시작)
			if (!Arguments.empty() && Arguments[0] == '"')
			{
				size_t FirstQuoteEnd = Arguments.find('"', 1);
				if (FirstQuoteEnd != FString::npos)
				{
					FString FormatString = Arguments.substr(1, FirstQuoteEnd - 1);

					// 뒤쪽에 있는 인수들 처리
					FString RemainingArguments;
					if (FirstQuoteEnd + 1 < Arguments.length())
					{
						RemainingArguments = Arguments.substr(FirstQuoteEnd + 1);
						// 앞의 콤마와 공백 제거
						while (!RemainingArguments.empty() &&
							(RemainingArguments[0] == ',' || RemainingArguments[0] == ' '))
							RemainingArguments = RemainingArguments.substr(1);
					}

					// printf 스타일 다중 인자 처리 개선
					if (!RemainingArguments.empty())
					{
						try
						{
							// 인자들을 콤마로 분리
							TArray<FString> Args;
							std::istringstream ArgStream(RemainingArguments);
							FString Token;

							// 콤마로 분리하되 따옴표 안의 콤마는 무시
							bool bInQuotes = false;
							FString CurrentArg;

							for (size_t i = 0; i < RemainingArguments.length(); ++i)
							{
								char Character = RemainingArguments[i];

								if (Character == '"')
								{
									bInQuotes = !bInQuotes;
									CurrentArg += Character;
								}
								else if (Character == ',' && !bInQuotes)
								{
									// 인자 완료
									// 앞뒤 공백 제거
									while (!CurrentArg.empty() && (CurrentArg.front() == ' ' || CurrentArg.front() ==
										'\t'))
									{
										CurrentArg = CurrentArg.substr(1);
									}
									while (!CurrentArg.empty() && (CurrentArg.back() == ' ' || CurrentArg.back() ==
										'\t'))
									{
										CurrentArg.pop_back();
									}

									if (!CurrentArg.empty())
									{
										Args.push_back(CurrentArg);
									}

									CurrentArg.clear();
								}
								else
								{
									CurrentArg += Character;
								}
							}

							// 마지막 인자 처리
							while (!CurrentArg.empty() && (CurrentArg.front() == ' ' || CurrentArg.front() == '\t'))
								CurrentArg = CurrentArg.substr(1);
							while (!CurrentArg.empty() && (CurrentArg.back() == ' ' || CurrentArg.back() == '\t'))
								CurrentArg.pop_back();
							if (!CurrentArg.empty())
								Args.push_back(CurrentArg);

							// 더 큰 버퍼로 안전하게 처리
							TArray<char> Buffer(4096);

							// 포맷 스트링의 플레이스홀더 개수와 인자 개수 확인
							size_t FormatSpecifiers = 0;
							size_t pos = 0;
							while ((pos = FormatString.find('%', pos)) != FString::npos)
							{
								if (pos + 1 < FormatString.length() && FormatString[pos + 1] != '%')
								{
									++FormatSpecifiers;
								}
								++pos;
							}

							if (FormatSpecifiers != Args.size())
							{
								FLogEntry ErrorEntry;
								ErrorEntry.Type = ELogType::Error;
								ErrorEntry.Message = "UE_LOG: 포맷 지정자(" + std::to_string(FormatSpecifiers) +
									")와 인자 개수(" + std::to_string(Args.size()) + ")가 일치하지 않습니다.";
								LogItems.push_back(ErrorEntry);
								bIsScrollToBottom = true;
								return;
							}

							// 실제 포맷팅 처리
							int Result = 0;

							if (Args.size() == 1)
							{
								if (FormatString.find("%d") != FString::npos || FormatString.find("%i") !=
									FString::npos)
								{
									int Value = std::stoi(Args[0]);
									Result = snprintf(Buffer.data(), Buffer.size(), FormatString.c_str(), Value);
								}
								else if (FormatString.find("%s") != FString::npos)
								{
									FString StringArg = Args[0];
									if (StringArg.length() >= 2 && StringArg.front() == '"' && StringArg.back() == '"')
										StringArg = StringArg.substr(1, StringArg.length() - 2);
									Result = snprintf(Buffer.data(), Buffer.size(), FormatString.c_str(),
									                  StringArg.c_str());
								}
								else if (FormatString.find("%f") != FString::npos)
								{
									float Value = std::stof(Args[0]);
									Result = snprintf(Buffer.data(), Buffer.size(), FormatString.c_str(), Value);
								}
							}
							else if (Args.size() == 2)
							{
								// 2개 인자 동시 처리 (가장 흔한 케이스들)
								size_t firstPercent = FormatString.find('%');
								size_t secondPercent = FormatString.find('%', firstPercent + 1);

								if (firstPercent != FString::npos && secondPercent != FString::npos)
								{
									char firstType = FormatString[firstPercent + 1];
									char secondType = FormatString[secondPercent + 1];

									// %d %d
									if ((firstType == 'd' || firstType == 'i') && (secondType == 'd' || secondType ==
										'i'))
									{
										int val1 = std::stoi(Args[0]);
										int val2 = std::stoi(Args[1]);
										Result = snprintf(Buffer.data(), Buffer.size(), FormatString.c_str(), val1,
										                  val2);
									}
									// %s %s
									else if (firstType == 's' && secondType == 's')
									{
										FString str1 = Args[0];
										FString str2 = Args[1];
										if (str1.length() >= 2 && str1.front() == '"' && str1.back() == '"')
											str1 = str1.substr(1, str1.length() - 2);
										if (str2.length() >= 2 && str2.front() == '"' && str2.back() == '"')
											str2 = str2.substr(1, str2.length() - 2);
										Result = snprintf(Buffer.data(), Buffer.size(), FormatString.c_str(),
										                  str1.c_str(), str2.c_str());
									}
									// %s %d 또는 %d %s
									else if ((firstType == 's' && (secondType == 'd' || secondType == 'i')) ||
										((firstType == 'd' || firstType == 'i') && secondType == 's'))
									{
										if (firstType == 's')
										{
											FString str = Args[0];
											if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
												str = str.substr(1, str.length() - 2);
											int val = std::stoi(Args[1]);
											Result = snprintf(Buffer.data(), Buffer.size(), FormatString.c_str(),
											                  str.c_str(), val);
										}
										else
										{
											int val = std::stoi(Args[0]);
											FString str = Args[1];
											if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
												str = str.substr(1, str.length() - 2);
											Result = snprintf(Buffer.data(), Buffer.size(), FormatString.c_str(), val,
											                  str.c_str());
										}
									}
								}
							}
							else
							{
								// 3개 이상 인자 또는 지원되지 않는 조합에 대한 일반적 처리
								FLogEntry ErrorEntry;
								ErrorEntry.Type = ELogType::Error;
								ErrorEntry.Message = "UE_LOG: " + std::to_string(Args.size()) +
									"개 인자 또는 현재 포맷 조합은 지원되지 않습니다. "
									"단순 형식만 사용해주세요 (1-2개 인자).";
								LogItems.push_back(ErrorEntry);
								bIsScrollToBottom = true;
								return;
							}

							// 포맷팅 결과 확인 및 안전성 검사
							if (Result < 0)
							{
								FLogEntry ErrorEntry;
								ErrorEntry.Type = ELogType::Error;
								ErrorEntry.Message = "UE_LOG: snprintf 포맷팅 오류가 발생했습니다.";
								LogItems.push_back(ErrorEntry);
								bIsScrollToBottom = true;
								return;
							}
							else if (Result >= static_cast<int>(Buffer.size()))
							{
								FLogEntry ErrorEntry;
								ErrorEntry.Type = ELogType::Error;
								ErrorEntry.Message = "UE_LOG: 출력이 버퍼 크기(" + std::to_string(Buffer.size()) +
									")를 초과했습니다. 필요한 크기: " + std::to_string(Result + 1);
								LogItems.push_back(ErrorEntry);
								bIsScrollToBottom = true;
								return;
							}

							// 성공적으로 포맷팅된 경우
							// Result는 null terminator를 제외한 길이이므로 Buffer[Result]는 안전함
							FLogEntry LogEntry;
							LogEntry.Type = ELogType::UELog;
							LogEntry.Message = FString(Buffer.data());
							LogItems.push_back(LogEntry);
							bIsScrollToBottom = true;
						}
						catch (const std::exception& e)
						{
							FLogEntry ErrorEntry;
							ErrorEntry.Type = ELogType::Error;
							ErrorEntry.Message = "UE_LOG: 인자 처리 중 오류 발생: " + FString(e.what());
							LogItems.push_back(ErrorEntry);
							bIsScrollToBottom = true;
						}
						catch (...)
						{
							FLogEntry ErrorEntry;
							ErrorEntry.Type = ELogType::Error;
							ErrorEntry.Message = "UE_LOG: 알 수 없는 인자 처리 오류가 발생했습니다: " + RemainingArguments;
							LogItems.push_back(ErrorEntry);
							bIsScrollToBottom = true;
						}
					}
					else
					{
						// 인수 없는 경우
						FLogEntry UELogEntry;
						UELogEntry.Type = ELogType::UELog;
						UELogEntry.Message = FormatString;
						LogItems.push_back(UELogEntry);
						bIsScrollToBottom = true;
					}
				}
				else
				{
					AddLog(ELogType::Error, "UELog: Error: Invalid Format: 문자열 종료 따옴표가 없습니다");
					bIsScrollToBottom = true;
				}
			}
			else
			{
				AddLog(ELogType::Error, "UELog: Error: Invalid Format: 첫 인자는 반드시 문자열이어야 합니다");
				bIsScrollToBottom = true;
			}
		}
		else
		{
			AddLog(ELogType::Error, "UELog: Error: Invalid Format: 맨 뒤 괄호가 존재하지 않습니다");
			bIsScrollToBottom = true;
		}
	}

	// Clear 명령어 입력
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "clear")
	{
		ClearLog();
	}

	// Help 명령어 입력
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "help")
	{
		AddLog(ELogType::System, "Available Commands:");
		AddLog(ELogType::Info, "  CLEAR - Clear The Console");
		AddLog(ELogType::Info, "  HELP - Show This Help");
		AddLog(ELogType::Info, "  UE_LOG(\"String with format\", Args...) - Log With printf Formatting");
		AddLog(ELogType::Debug, "    1개 인자 예제: UE_LOG(\"Hello World %%d\", 2025)");
		AddLog(ELogType::Debug, "    1개 인자 예제: UE_LOG(\"User: %%s\", \"John\")");
		AddLog(ELogType::Debug, "    2개 인자 예제: UE_LOG(\"Player %%s has %%d points\", \"Alice\", 1500)");
		AddLog(ELogType::Debug, "    2개 인자 예제: UE_LOG(\"Score: %%d, Lives: %%d\", 2500, 3)");
		AddLog(ELogType::Warning, "    주의: 포맷 지정자와 인자 개수가 일치해야 합니다!");
		AddLog(ELogType::Info, "");
		AddLog(ELogType::System, "Camera Controls:");
		AddLog(ELogType::Info, "  우클릭 + WASD - 카메라 이돘");
		AddLog(ELogType::Info, "  우클릭 + Q/E - 위/아래 이돘");
		AddLog(ELogType::Info, "  우클릭 + 마우스 이동 - 카메라 회전");
		AddLog(ELogType::Success, "  우클릭 + 마우스 휠 - 이동속도 조절 (0.5~50.0)");
		AddLog(ELogType::Info, "");
		AddLog(ELogType::System, "Terminal Commands:");
		AddLog(ELogType::Info, "  dir, ls - List directory contents");
		AddLog(ELogType::Info, "  cd [path] - Change directory");
		AddLog(ELogType::Info, "  echo [text] - Display text");
		AddLog(ELogType::Info, "  type [file] - Display file contents");
		AddLog(ELogType::Info, "  ping [host] - Network ping");
		AddLog(ELogType::Info, "  Any Windows command will be executed directly");
	}
	else
	{
		// 실제 터미널 명령어 실행
		ExecuteTerminalCommand(InCommand);
	}

	// 스크롤 하단으로 이동
	bIsScrollToBottom = true;
}

/**
 * @brief 실제 터미널 명령어를 실행하고 결과를 콘솔에 표시하는 함수
 * @param InCommand 실행할 터미널 명령어
 */
void UConsoleWidget::ExecuteTerminalCommand(const char* InCommand)
{
	if (!InCommand || strlen(InCommand) == 0)
	{
		return;
	}

	AddLog("[Terminal] > %s", InCommand);

	try
	{
		FString FullCommand = "powershell /c " + FString(InCommand);

		// Prepare Pipe for Create Process
		HANDLE PipeReadHandle, PipeWriteHandle;

		SECURITY_ATTRIBUTES SecurityAttribute = {};
		SecurityAttribute.nLength = sizeof(SECURITY_ATTRIBUTES);
		SecurityAttribute.bInheritHandle = TRUE;
		SecurityAttribute.lpSecurityDescriptor = nullptr;

		if (!CreatePipe(&PipeReadHandle, &PipeWriteHandle, &SecurityAttribute, 0))
		{
			AddLog("[Terminal Error] Failed To Create Pipe For Command Execution.");
			return;
		}

		// Set STARTUPINFO
		STARTUPINFOA StartUpInfo = {};
		StartUpInfo.cb = sizeof(STARTUPINFOA);
		StartUpInfo.dwFlags |= STARTF_USESTDHANDLES;
		StartUpInfo.hStdInput = nullptr;

		// stderr & stdout을 동일 파이프에서 처리
		StartUpInfo.hStdError = PipeWriteHandle;
		StartUpInfo.hStdOutput = PipeWriteHandle;

		PROCESS_INFORMATION ProcessInformation = {};

		// lpCommandLine Param Setting
		TArray<char> CommandLine(FullCommand.begin(), FullCommand.end());
		CommandLine.push_back('\0');

		// Create Process
		if (!CreateProcessA(nullptr, CommandLine.data(), nullptr,
		                    nullptr, TRUE, CREATE_NO_WINDOW,
		                    nullptr, nullptr, &StartUpInfo, &ProcessInformation))
		{
			AddLog("ConsoleWindow: Error: Failed To Execute Command: %s", InCommand);
			CloseHandle(PipeReadHandle);
			CloseHandle(PipeWriteHandle);
			return;
		}

		// 부모 프로세스에서는 쓰기 핸들이 불필요하므로 바로 닫도록 처리
		CloseHandle(PipeWriteHandle);

		// Read Execution Process With Timeout
		char Buffer[256];
		DWORD ReadDoubleWord;
		FString Output;
		bool bHasOutput = false;

		DWORD StartTime = GetTickCount();
		const DWORD TimeoutMsec = 2000;
		while (true)
		{
			// 타임아웃 체크
			if (GetTickCount() - StartTime > TimeoutMsec)
			{
				// 프로세스가 아직 실행 중이면 강제 종료
				DWORD ProcessExitCode;
				if (GetExitCodeProcess(ProcessInformation.hProcess, &ProcessExitCode) && ProcessExitCode ==
					STILL_ACTIVE)
				{
					TerminateProcess(ProcessInformation.hProcess, 1);
				}
				AddLog("ConsoleWindow: Error: Reading Timeout");
				break;
			}

			// 프로세스가 아직 실행 중인지 확인
			DWORD ProcessExitCode;
			if (GetExitCodeProcess(ProcessInformation.hProcess, &ProcessExitCode) && ProcessExitCode != STILL_ACTIVE)
			{
				// 프로세스가 종료되었으면 남은 모든 데이터를 읽고 종료
				DWORD AvailableBytes = 0;
				while (PeekNamedPipe(PipeReadHandle, nullptr, 0, nullptr, &AvailableBytes, nullptr) && AvailableBytes >
					0)
				{
					if (ReadFile(PipeReadHandle, Buffer, sizeof(Buffer) - 1,
					             &ReadDoubleWord, nullptr) && ReadDoubleWord > 0)
					{
						Buffer[ReadDoubleWord] = '\0';
						Output += Buffer;
						bHasOutput = true;
					}
					else
					{
						break; // ReadFile 실패 시 종료
					}
				}
				break;
			}

			// 파이프에 데이터가 있는지 확인
			DWORD AvailableBytes = 0;
			if (!PeekNamedPipe(PipeReadHandle, nullptr, 0, nullptr, &AvailableBytes, nullptr) || AvailableBytes == 0)
			{
				// 데이터가 없으면 잠시 대기
				Sleep(100);
				continue;
			}

			// 데이터 읽기
			if (ReadFile(PipeReadHandle, Buffer, sizeof(Buffer) - 1, &ReadDoubleWord, nullptr) && ReadDoubleWord > 0)
			{
				Buffer[ReadDoubleWord] = '\0'; // Null 문자로 String 끝 세팅
				Output += Buffer;
				bHasOutput = true;
				StartTime = GetTickCount();
			}
			else
			{
				// ReadFile 실패시 종료
				break;
			}
		}

		// Process 종료 대기
		DWORD WaitResult = WaitForSingleObject(ProcessInformation.hProcess, 2000);

		// 종료 코드 확인
		DWORD ExitCode = 0;
		if (WaitResult == WAIT_TIMEOUT)
		{
			// 타임아웃 발생 시 프로세스 강제 종료
			TerminateProcess(ProcessInformation.hProcess, 1);
			AddLog("ConsoleWindow: Error: Command Timeout & Terminated");
			ExitCode = 1;
		}
		else
		{
			GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);
		}

		// Release Handle
		CloseHandle(PipeReadHandle);
		CloseHandle(ProcessInformation.hProcess);
		CloseHandle(ProcessInformation.hThread);

		// Print Result
		if (bHasOutput)
		{
			FString Utf8Output = ConvertCP949ToUTF8(Output.c_str());

			std::istringstream Stream(Utf8Output);
			FString Line;
			while (std::getline(Stream, Line))
			{
				// istringstream이 \r을 남길 수 있으므로 제거
				if (!Line.empty() && Line.back() == '\r')
				{
					Line.pop_back();
				}
				if (!Line.empty())
				{
					AddLog("%s", Line.c_str());
				}
			}
		}
		else if (ExitCode == 0)
		{
			// 출력은 없지만 성공적으로 실행된 경우
			AddLog("[Terminal] Command executed successfully (no output)");
		}

		// 에러 코드가 있는 경우 표시
		if (ExitCode != 0)
		{
			AddLog("[Terminal Error] Command failed with exit code: %d", ExitCode);
		}
	}
	catch (const exception& Exception)
	{
		AddLog("[Terminal Error] Exception occurred: %s", Exception.what());
	}
	catch (...)
	{
		AddLog("[Terminal Error] Unknown Error Occurred While Executing Command");
	}
}

// System output redirection implementation
void UConsoleWidget::InitializeSystemRedirect()
{
	try
	{
		// Save Original Stream Buffers
		OriginalConsoleOutput = cout.rdbuf();
		OriginalConsoleError = cerr.rdbuf();

		// Create Custom Stream Buffers
		ConsoleOutputBuffer = new ConsoleStreamBuffer(this, false);
		ConsoleErrorBuffer = new ConsoleStreamBuffer(this, true);

		// Redirect ConsoleOutput and ConsoleError To Our Custom Buffers
		cout.rdbuf(ConsoleOutputBuffer);
		cerr.rdbuf(ConsoleErrorBuffer);

		AddLog(ELogType::System, "ConsoleWindow: Console Output Redirection Initialized");
	}
	catch (...)
	{
		AddLog(ELogType::Error, "ConsoleWindow: Failed To Initialize Console Output Redirection");
	}
}

void UConsoleWidget::CleanupSystemRedirect()
{
	try
	{
		// Restore Original Stream Buffers
		if (OriginalConsoleOutput)
		{
			cout.rdbuf(OriginalConsoleOutput);
			OriginalConsoleOutput = nullptr;
		}

		if (OriginalConsoleError)
		{
			cerr.rdbuf(OriginalConsoleError);
			OriginalConsoleError = nullptr;
		}

		// Delete Custom Stream Buffers
		if (ConsoleOutputBuffer)
		{
			delete ConsoleOutputBuffer;
			ConsoleOutputBuffer = nullptr;
		}

		if (ConsoleErrorBuffer)
		{
			delete ConsoleErrorBuffer;
			ConsoleErrorBuffer = nullptr;
		}
	}
	catch (...)
	{
		// Ignore Cleanup Errors
	}
}

void UConsoleWidget::AddSystemLog(const char* InText, bool bInIsError)
{
	if (!InText || strlen(InText) == 0)
	{
		return;
	}

	FString LogText;
	ELogType LogType; 

	if (bInIsError)
	{
		LogText = "Error: " + FString(InText);
		LogType = ELogType::Error;
	}
	else
	{
		LogText = "System: " + FString(InText);
		LogType = ELogType::System;
	}

	// 끝에 있는 개행 문자 제거
	if (!LogText.empty() && LogText.back() == '\n')
	{
		LogText.pop_back();
	}

	FLogEntry LogEntry;
	LogEntry.Type = LogType;
	LogEntry.Message = LogText;
	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}
