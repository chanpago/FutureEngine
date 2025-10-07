#include "pch.h"
#include "Render/UI/Widget/SceneIOWidget.h"
#include <shobjidl.h>
#include "Manager/Level/World.h"

IMPLEMENT_CLASS(USceneIOWidget, UWidget)

USceneIOWidget::USceneIOWidget()
{
}

USceneIOWidget::~USceneIOWidget() = default;

void USceneIOWidget::Initialize()
{
}

void USceneIOWidget::Update()
{
}

void USceneIOWidget::RenderWidget()
{
	// Save Section
	ImGui::Text("Scene Save & Load");
	ImGui::Spacing();

	if (ImGui::Button("Save Scene", ImVec2(90, 20)))
	{
		path FilePath = OpenSaveFileDialog();
		if (!FilePath.empty())
		{
			SaveLevel(FilePath.string());
		}
	}

	// ImGui::SameLine();
	// if (ImGui::Button("Quick Save", ImVec2(120, 30)))
	// {
	// 	// 빠른 저장 - 기본 경로에 저장
	// 	SaveLevel("");
	// }

	// Load Section
	ImGui::SameLine();
	if (ImGui::Button("Load Scene", ImVec2(90, 20)))
	{
		path FilePath = OpenLoadFileDialog();
		if (!FilePath.empty())
		{
			LoadLevel(FilePath.string());
		}
	}

	ImGui::Spacing();

	// New Level Section
	ImGui::Text("새로운 Scene 생성");
	ImGui::Spacing();

	ImGui::InputText("Level Name", NewLevelNameBuffer, sizeof(NewLevelNameBuffer));
	if (ImGui::Button("Create New Scene", ImVec2(120, 25)))
	{
		CreateNewLevel();
	}

	// Status Message
	if (StatusMessageTimer > 0.0f)
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), StatusMessage.c_str());
		StatusMessageTimer -= DT;
	}
	// Reserve Space
	else
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
	}

	ImGui::Separator();
}

/**
 * @brief 지정된 경로에 Level을 Save하는 UI Window 기능
 * @param InFilePath 저장할 파일 경로
 */
void USceneIOWidget::SaveLevel(const FString& InFilePath)
{ 
	try
	{
		bool bSuccess;

		if (InFilePath.empty())
		{
			// Quick Save인 경우 기본 경로 사용
			bSuccess = GWorld->SaveCurrentLevel("");
		}
		else
		{
			bSuccess = GWorld->SaveCurrentLevel(InFilePath);
		}

		if (bSuccess)
		{
			StatusMessage = "Level Saved Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: Level Saved Successfully");
		}
		else
		{
			StatusMessage = "Failed To Save Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: Failed To Save Level");
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Save Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		UE_LOG("SceneIO: Save Error: %s", Exception.what());
	}
}

/**
 * @brief 지정된 경로에서 Level File을 Load 하는 UI Window 기능
 * @param InFilePath 불러올 파일 경로
 */
void USceneIOWidget::LoadLevel(const FString& InFilePath)
{
	try
	{
		// 파일명에서 확장자를 제외하고 레벨 이름 추출
		FString LevelName = InFilePath;

		size_t LastSlash = LevelName.find_last_of("\\/");
		if (LastSlash != std::wstring::npos)
		{
			LevelName = LevelName.substr(LastSlash + 1);
		}

		size_t LastDot = LevelName.find_last_of(".");
		if (LastDot != std::wstring::npos)
		{
			LevelName = LevelName.substr(0, LastDot);
		}
		 
		bool bSuccess = GWorld->LoadLevel(LevelName, InFilePath);

		if (bSuccess)
		{
			StatusMessage = "레벨을 성공적으로 로드했습니다";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
		}
		else
		{
			StatusMessage = "레벨을 로드하는 데에 실패했습니다";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
		}

		UE_LOG("SceneIO: %s", StatusMessage.c_str());
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Load Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		UE_LOG("SceneIO: Load Error: %s",Exception.what());
	}
}

/**
 * @brief New Blank Level을 생성하는 UI Window 기능
 */
void USceneIOWidget::CreateNewLevel()
{
	try
	{
		FString LevelName = FString(NewLevelNameBuffer, NewLevelNameBuffer + strlen(NewLevelNameBuffer));

		if (LevelName.empty())
		{
			StatusMessage = "Please Enter A Level Name!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			return;
		}
		 
		bool bSuccess = GWorld->CreateNewLevel(LevelName);

		if (bSuccess)
		{
			StatusMessage = "New Level Created Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: New Level Created: %s", FString(NewLevelNameBuffer).c_str());
		}
		else
		{
			StatusMessage = "Failed To Create New Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: Failed To Create New Level");
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = std::string("Create Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		UE_LOG("SceneIO: Create Error: ");
	}
}

/**
 * @brief Windows API를 활용한 파일 저장 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path USceneIOWidget::OpenSaveFileDialog()
{
	path ResultPath = L"";
	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(ResultHandle))
	{
		IFileSaveDialog* FileSaveDialogPtr = nullptr;
		// 2. FileSaveDialog 인스턴스 생성
		ResultHandle = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
			IID_IFileSaveDialog, reinterpret_cast<void**>(&FileSaveDialogPtr));
		if (SUCCEEDED(ResultHandle))
		{
			// 3. 대화상자 옵션 설정
			// 파일 타입 필터 설정
			COMDLG_FILTERSPEC SpecificationRange[] = {
				{L"Scene Files (*.Scene)", L"*.Scene"},
				{L"All Files (*.*)", L"*.*"}
			};
			FileSaveDialogPtr->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);
			// 기본 필터를 "JSON Files" 로 설정
			FileSaveDialogPtr->SetFileTypeIndex(1);
			// 기본 확장자 설정
			FileSaveDialogPtr->SetDefaultExtension(L"Scene");
			// 대화상자 제목 설정
			FileSaveDialogPtr->SetTitle(L"Save Level File");
			// Set Flag
			DWORD DoubleWordFlags;
			FileSaveDialogPtr->GetOptions(&DoubleWordFlags);
			FileSaveDialogPtr->SetOptions(DoubleWordFlags | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);
			// Show Modal
			// 현재 활성 창을 부모로 가짐
			UE_LOG("SceneIO: Save Dialog Modal Opening...");
			ResultHandle = FileSaveDialogPtr->Show(GetActiveWindow());
			// 결과 처리
			// 사용자가 '저장' 을 눌렀을 경우
			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG("SceneIO: Save Dialog Modal Closed - 파일 선택됨");
				IShellItem* ShellItemResult;
				ResultHandle = FileSaveDialogPtr->GetResult(&ShellItemResult);
				if (SUCCEEDED(ResultHandle))
				{
					// Get File Path from IShellItem
					PWSTR FilePath = nullptr;
					ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);
					if (SUCCEEDED(ResultHandle))
					{
						ResultPath = path(FilePath);
						CoTaskMemFree(FilePath);
					}
					ShellItemResult->Release();
				}
			}
			// 사용자가 '취소'를 눌렀거나 오류 발생
			else
			{
				UE_LOG("SceneIO: Save Dialog Modal Closed - 취소됨");
			}
			// Release FileSaveDialog
			FileSaveDialogPtr->Release();
		}
		// COM 라이브러리 해제
		CoUninitialize();
	}
	return ResultPath;
}
/**
 * @brief Windows API를 활용한 파일 로드 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path USceneIOWidget::OpenLoadFileDialog()
{
	path ResultPath = L"";
	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(ResultHandle))
	{
		IFileOpenDialog* FileOpenDialog = nullptr;
		// FileOpenDialog 인스턴스 생성
		ResultHandle = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&FileOpenDialog));
		if (SUCCEEDED(ResultHandle))
		{
			// 파일 타입 필터 설정
			COMDLG_FILTERSPEC SpecificationRange[] = {
				{L"Scene Files (*.Scene)", L"*.Scene"},
				{L"All Files (*.*)", L"*.*"}
			};
			FileOpenDialog->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);
			// 기본 필터를 "JSON Files" 로 설정
			FileOpenDialog->SetFileTypeIndex(1);
			// 대화상자 제목 설정
			FileOpenDialog->SetTitle(L"Load Level File");
			// Flag Setting
			DWORD DoubleWordFlags;
			FileOpenDialog->GetOptions(&DoubleWordFlags);
			FileOpenDialog->SetOptions(DoubleWordFlags | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
			// Open Modal
			UE_LOG("SceneIO: Load Dialog Modal Opening...");
			ResultHandle = FileOpenDialog->Show(GetActiveWindow()); // 현재 활성 창을 부모로
			// 결과 처리
			// 사용자가 '열기' 를 눌렀을 경우
			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG("SceneIO: Load Dialog Modal Closed - 파일 선택됨");
				IShellItem* ShellItemResult;
				ResultHandle = FileOpenDialog->GetResult(&ShellItemResult);
				if (SUCCEEDED(ResultHandle))
				{
					// Get File Path from IShellItem
					PWSTR ReturnFilePath = nullptr;
					ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &ReturnFilePath);
					if (SUCCEEDED(ResultHandle))
					{
						ResultPath = path(ReturnFilePath);
						CoTaskMemFree(ReturnFilePath);
					}
					ShellItemResult->Release();
				}
			}
			// 사용자가 '취소' 를 눌렀거나 오류 발생
			else
			{
				UE_LOG("SceneIO: Load Dialog Modal Closed - 취소됨");
			}
			FileOpenDialog->Release();
		}
		// COM 라이브러리 해제
		CoUninitialize();
	}
	return ResultPath;
}
