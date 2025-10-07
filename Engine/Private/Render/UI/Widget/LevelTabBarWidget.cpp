#include "pch.h"
#include "public/Render/UI/Widget/LevelTabBarWidget.h"
#include "Public/Manager/UI/UIManager.h"
#include "Public/Render/UI/Window/UIWindow.h"
#include "public/Manager/Viewport/ViewportManager.h"
#include <d3d11.h>
#include "DirectXTK/Inc/WICTextureLoader.h"
#include "Render/Renderer/Renderer.h"
#include <shobjidl.h>
#include "Manager/Level/World.h"
#include "Editor/EditorEngine.h"
#include "Actor/StaticMeshActor.h"
#include "Global/NameTable.h"

IMPLEMENT_CLASS(ULevelTabBarWidget, UWidget)


ULevelTabBarWidget::ULevelTabBarWidget()
{
	UIManager = &UUIManager::GetInstance();
	if (!UIManager)
	{
		UE_LOG("MainBarWidget: UIManager를 찾을 수 없습니다!");
		return;
	}

	UE_LOG("MainBarWidget: 레벨 텝바 위젯이 초기화되었습니다");


	// 초기화 시
	DirectX::CreateWICTextureFromFile(URenderer::GetInstance().GetDevice(),L"Asset/logo/EngineCHLogo.png",nullptr,LeftIconSRV.GetAddressOf());

	DirectX::CreateWICTextureFromFile(URenderer::GetInstance().GetDevice(), L"Asset/LevelBarIcon/Load.png", nullptr, LoadIconSRV.GetAddressOf());
	DirectX::CreateWICTextureFromFile(URenderer::GetInstance().GetDevice(), L"Asset/LevelBarIcon/Save.png", nullptr, SaveIconSRV.GetAddressOf());
	DirectX::CreateWICTextureFromFile(URenderer::GetInstance().GetDevice(), L"Asset/LevelBarIcon/New.png", nullptr, NewIconSRV.GetAddressOf());

	DirectX::CreateWICTextureFromFile(URenderer::GetInstance().GetDevice(), L"Asset/LevelBarIcon/CreateActor.png", nullptr, CreateActorIconSRV.GetAddressOf());


	DirectX::CreateWICTextureFromFile(URenderer::GetInstance().GetDevice(), L"Asset/LevelBarIcon/Play.png", nullptr, PlayPIEIconSRV.GetAddressOf());
	DirectX::CreateWICTextureFromFile(URenderer::GetInstance().GetDevice(), L"Asset/LevelBarIcon/Stop.png", nullptr, StopPIEIconSRV.GetAddressOf());

}

ULevelTabBarWidget::~ULevelTabBarWidget()
{
}

void ULevelTabBarWidget::Initialize()
{
}

void ULevelTabBarWidget::Update()
{
}

void ULevelTabBarWidget::RenderWidget()
{
	ImGuiViewport* vp = ImGui::GetMainViewport();
	
	// ① 메뉴바 높이 (MainBarWidget에서 계산해 UIManager에 저장했다고 가정)
	// 20px 부터 그려짐
	const float mainBarH = 20.0f;

	// ② 탭 스트립 높이
	// 36의 높이를 가지고 있음


	const float leftGap = 60.0f; // 원하는 틈(px)


	// 화면(메인 뷰) 기준으로 절대 배치
	const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(0.0f, mainBarH));
	ImGui::SetNextWindowSize(ImVec2(screenSize.x, TabStripHeight));
	
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));


	if (ImGui::Begin("LevelTabStrip", nullptr, flags))
	{
		// === [여기] PNG를 왼쪽 여백(leftGap) 영역에 그리기 ===
		if (LeftIconSRV)               // ID3D11ShaderResourceView* 보관해 둔 것
		{
			ImDrawList* fg = ImGui::GetForegroundDrawList();  // 최상단 레이어
			float x = 8.0f, y = 5.0f;                         // 화면(뷰포트) 좌상단 기준 좌표
			float s = 45.0f;                                   // 아이콘 크기
			fg->AddImage((ImTextureID)LeftIconSRV.Get(),
				ImVec2(x, y), ImVec2(x + s, y + s));  // 메뉴바 위에 겹쳐 그려짐
		}
		// ===================================================


		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + leftGap);
	
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));
		ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);  // ★ 탭바 하단 라인 제거

		// 회색 계열 탭 색상
		ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.06f, 0.06f, 0.07f, 1.0f)); // 기본(더 어두운 회색)
		ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.22f, 0.22f, 0.24f, 1.0f)); // 호버
		ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); // 선택
		ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.06f, 0.06f, 0.07f, 1.0f)); // 비활성
		ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.22f, 0.22f, 0.24f, 1.0f)); // 비활성+선택

		if (ImGui::BeginTabBar("LevelTabs",
			ImGuiTabBarFlags_Reorderable |
			ImGuiTabBarFlags_FittingPolicyScroll |
			ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
		{
			if (ImGui::BeginTabItem("CurrentLevel")) { ImGui::EndTabItem(); }
			// ... 더 많은 탭 ...
			ImGui::EndTabBar();
		}
	
		// 색/스타일 복원
		ImGui::PopStyleColor(5); 
		ImGui::PopStyleVar(3);
	}
	ImGui::End();
	
	ImGui::PopStyleColor(1);
	ImGui::PopStyleVar(3);

	{
		//const float y = mainBarH + tabStripH - 2.0f;       // 선이 보이는 위치
		//ImDrawList* dl = ImGui::GetForegroundDrawList();
		//dl->AddRectFilled(ImVec2(0.0f, y),ImVec2(screenSize.x, y + 2.0f), ImGui::GetColorU32(ImVec4(0.06f, 0.06f, 0.07f, 1.0f)));              // 툴바와 같은 색으로 덮기

	}


	////////////////////////////////////////////////////////////////////////////////////////////////
	// 지금 부터 레벨 탭바 아래 칸 생성
	// ─────────────────────────────────────
	// (2) 툴 칸 (탭 스트립 바로 아래)  ← 여기 추가
	// ─────────────────────────────────────                                  // 요청한 높이
	const ImVec4 toolBg = ImVec4(0.06f, 0.06f, 0.07f, 1.0f);      // TabUnfocused와 동일 톤

	ImGui::SetNextWindowPos(ImVec2(0.0f, mainBarH + LevelBarHeight));
	ImGui::SetNextWindowSize(ImVec2(screenSize.x, LevelBarHeight));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, toolBg);

	if (ImGui::Begin("LevelToolBar", nullptr, flags))
	{
		// 배경/호버/액티브(hover/active)는 스타일 컬러를 씀
		const ImVec4 colBtn = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
		const ImVec4 colBtnHover = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
		const ImVec4 colBtnActive = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);
		const ImU32 separatorColor = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Separator));

		auto IconButton = [&](const char* id, ID3D11ShaderResourceView* srv, const char* tooltip) -> bool
			{
				if (!srv) return ImGui::Button(id, ImVec2(30, 30));

				// ⚠️ ImTextureRef 로 변환: 프로젝트 타입에 맞게 필요시 수정
				ImTextureRef tex_ref = (ImTextureRef)srv;  // 혹은 ImTextureRef(texID) 등

				ImGui::PushStyleColor(ImGuiCol_Button, colBtn);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colBtnHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, colBtnActive);

				// 네 시그니처에 정확히 맞춰 호출
				bool clicked = ImGui::ImageButton(
					id,                     // const char* str_id
					tex_ref,                // ImTextureRef
					ImVec2(20, 20),         // image_size
					ImVec2(0, 0),           // uv0
					ImVec2(1, 1),           // uv1
					ImVec4(0, 0, 0, 0),     // bg_col (투명 배경, 필요시 colBtn로 변경)
					ImVec4(1, 1, 1, 1)      // tint_col (흰색 = 원본 그대로)
				);

				if (ImGui::IsItemHovered() && tooltip)
					ImGui::SetTooltip("%s", tooltip);

				ImGui::PopStyleColor(3);
				return clicked;
			};

		// --- 버튼들 ---
		if (IconButton("##btn_load", LoadIconSRV.Get(), "Load Level"))
		{
			std::filesystem::path FilePath = OpenLoadFileDialog();
			if (!FilePath.empty())
				LoadLevel(FilePath.string());
		}

		ImGui::SameLine(0.0f, 10.0f);

		if (IconButton("##btn_save", SaveIconSRV.Get(), "Save Level"))
		{
			std::filesystem::path FilePath = OpenSaveFileDialog();
			if (!FilePath.empty())
				SaveLevel(FilePath.string());
			// 퀵세이브 원하면 else SaveLevel("");
		}

		ImGui::SameLine(0.0f, 10.0f);

		if (IconButton("##btn_new", NewIconSRV.Get(), "New Level"))
		{
			CreateNewLevel();
		}

		ToolbarVSeparator(10.0f, 6.0f, 6.0f, 1.0f, separatorColor, 5.0f);

		if (IconButton("##btn_Create", CreateActorIconSRV.Get(), "Create Actor"))
		{
			ImGui::OpenPopup("CreateActorMenu"); // ← 팝업 열기
		}

		// 팝업 메뉴: 여기서 MenuItem들을 사용
		if (ImGui::BeginPopup("CreateActorMenu"))
		{
			if (ImGui::MenuItem("Actor"))
			{
				GWorld->GetCurrentLevel()->SpawnActor<AActor>("Actor");
			}
			if (ImGui::MenuItem("Static Mesh Actor"))
			{
				GWorld->GetCurrentLevel()->SpawnActor<AStaticMeshActor>("StaticMeshActor");
			}

			// 추후에 다른 액터 추가용 코드 
			//ImGui::Separator();
			//
			//if (ImGui::BeginMenu("Lights"))
			//{
			//	if (ImGui::MenuItem("Point Light")) {  }
			//	if (ImGui::MenuItem("Directional Light")) { }
			//	if (ImGui::MenuItem("Spot Light")) {  }
			//	ImGui::EndMenu();
			//}
			//
			//ImGui::Separator();
			//
			//if (ImGui::BeginMenu("Shapes"))
			//{
			//	if (ImGui::MenuItem("Cube")) { }
			//	if (ImGui::MenuItem("Sphere")) { }
			//	if (ImGui::MenuItem("Plane")) {  }
			//	if (ImGui::MenuItem("Cylinder")) { }
			//	ImGui::EndMenu();
			//}

			ImGui::EndPopup();
		}

		ToolbarVSeparator(10.0f, 6.0f, 6.0f, 1.0f, separatorColor, 5.0f);

		if (IconButton("##btn_StartPIE", PlayPIEIconSRV.Get(), "Play PIE"))
		{
			StartPIE();
		}
		ImGui::SameLine(0.0f, 10.0f);

		if (IconButton("##btn_StopPIE", StopPIEIconSRV.Get(), "Stop PIE"))
		{
			EndPIE();
		}


	}
	ImGui::End();

	ImGui::PopStyleColor(1);
	ImGui::PopStyleVar(3);
}

float ULevelTabBarWidget::GetLevelBarHeight() const
{
	return TotalLevelBarHeight;
}

/**
 * @brief 지정된 경로에 Level을 Save하는 UI Window 기능
 * @param InFilePath 저장할 파일 경로
 */

void ULevelTabBarWidget::SaveLevel(const FString& InFilePath)
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
void ULevelTabBarWidget::LoadLevel(const FString& InFilePath)
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
		UE_LOG("SceneIO: Load Error: %s", Exception.what());
	}
}

void ULevelTabBarWidget::CreateNewLevel()
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

void ULevelTabBarWidget::StartPIE()
{
	if (PIEUIState == EPIEUIState::Playing) return;

	PIEUIState = EPIEUIState::Playing;

	FNameTable::GetInstance().PushNumberMapState();

	//TODO: NO Current Level
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	UWorld* PIEWorld = UWorld::DuplicateWorldForPIE(EditorWorld);

	if (!PIEWorld)
	{
		PIEUIState = EPIEUIState::Stopped;
		FNameTable::GetInstance().PopNumberMapState();
		return;
	}

	GWorld = PIEWorld;
	PIEWorld->InitializeActorsForPlay();

	UUIManager::GetInstance().SetSelectedActor(nullptr);
}

void ULevelTabBarWidget::EndPIE()
{
	if (PIEUIState == EPIEUIState::Stopped) return;

	PIEUIState = EPIEUIState::Stopped;

	if (GWorld && GWorld->IsPIEWorld())
	{
		GWorld->CleanUpWorld();
		delete GWorld;
	}

	GWorld = GEditor->GetEditorWorldContext().World();
	GEditor->RemoveWorldContext(EWorldType::PIE);

	FNameTable::GetInstance().PopNumberMapState();

	UUIManager::GetInstance().SetSelectedActor(nullptr);
}

path ULevelTabBarWidget::OpenSaveFileDialog()
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

path ULevelTabBarWidget::OpenLoadFileDialog()
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

void ULevelTabBarWidget::ToolbarVSeparator(float post_spacing, float top_margin_px, float bottom_margin_px, float thickness, ImU32 col, float x_offset)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 wp = ImGui::GetWindowPos();
	const ImVec2 wsz = ImGui::GetWindowSize();

	ImGui::SameLine(0.0f, 0.0f);
	ImVec2 cur = ImGui::GetCursorScreenPos();

	// 마지막 아이템의 오른쪽 끝 vs 현재 커서 x 중 오른쪽 값
	float x = std::max(ImGui::GetItemRectMax().x, cur.x);

	// 버튼 프레임(보더) 두께만큼 밀어준다 → 이게 없으면 2줄처럼 보임
	x += ImGui::GetStyle().FrameBorderSize; // 보통 0 또는 1

	x = std::floor(x + x_offset);
	const float y1 = std::floor(wp.y + top_margin_px);
	const float y2 = std::floor(wp.y + wsz.y - bottom_margin_px);

	if (col == 0) col = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
	dl->AddRectFilled(ImVec2(x, y1), ImVec2(x + thickness, y2), col);

	// 레이아웃 진행
	ImGui::SetCursorScreenPos(ImVec2(x, cur.y));
	ImGui::Dummy(ImVec2(thickness, 0.0f));
	ImGui::SameLine(0.0f, post_spacing);
}
