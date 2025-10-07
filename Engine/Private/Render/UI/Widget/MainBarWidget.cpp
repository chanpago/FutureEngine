#include "pch.h"
#include "public/Render/UI/Widget/MainBarWidget.h"
#include "Public/Manager/UI/UIManager.h"
#include "Public/Render/UI/Window/UIWindow.h"
#include "public/Manager/Viewport/ViewportManager.h"
#include "DirectXTK/Inc/WICTextureLoader.h"
#include "Render/Renderer/Renderer.h"
#include "Manager/Level/World.h"
#include "Level/Level.h"

IMPLEMENT_CLASS(UMainBarWidget, UWidget)

UMainBarWidget::UMainBarWidget() : UWidget("MainBarWidget")
{
}

void UMainBarWidget::Initialize()
{
	UIManager = &UUIManager::GetInstance();
	if (!UIManager)
	{
		UE_LOG("MainBarWidget: UIManager를 찾을 수 없습니다!");
		return;
	}

	UE_LOG("MainBarWidget: 메인 메뉴바 위젯이 초기화되었습니다");

	LoadLogo(URenderer::GetInstance().GetDevice());
}

void UMainBarWidget::Update()
{
}


// 20px정도의 높이
void UMainBarWidget::RenderWidget()
{
	if (!bIsMenuBarVisible)
	{
		MenuBarHeight = 0.0f;
		return;
	}

	// 기본 스타일만 살짝 손봄(높이 키우지 않음)
	ImGuiStyle& st = ImGui::GetStyle();
	const float origPadX = st.FramePadding.x;
	const float origPadY = st.FramePadding.y;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(origPadX, origPadY));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, st.ItemSpacing.y));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));  // 보험용(투명)

	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0, 0, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.11f, 0.11f, 0.12f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.23f, 0.23f, 0.26f, 1.0f));

	if (ImGui::BeginMainMenuBar())
	{
		// 현재 메뉴바 높이 기록(다른 레이아웃에서 필요하면 사용)
		MenuBarHeight = ImGui::GetWindowSize().y;

		// ★ 60px 더미를 두고 같은 줄에서 계속 그리기
		ImGui::Dummy(ImVec2(60.0f, 0.0f));
		ImGui::SameLine(0.0f, 0.0f);

		// 로고/더미/오프셋 전부 제거 — 바로 메뉴만 렌더
		RenderFileMenu();
		RenderShowFlagsMenu();
		RenderWindowsMenu();
		RenderHelpMenu();

		ImGui::EndMainMenuBar();
	}

	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar(4);
}

void UMainBarWidget::RenderFileMenu()
{
	if (ImGui::BeginMenu("파일"))
	{
		// 레벨 관련 메뉴
		if (ImGui::MenuItem("새 레벨", "Ctrl+N"))
		{
			CreateNewLevel();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("레벨 열기", "Ctrl+O"))
		{
			LoadLevel();
		}

		if (ImGui::MenuItem("레벨 저장", "Ctrl+S"))
		{
			SaveCurrentLevel();
		}

		ImGui::Separator();

		// 일반 파일 작업
		if (ImGui::MenuItem("일반 파일 열기"))
		{
			UE_LOG("MainBarWidget: 일반 파일 열기 메뉴 선택됨");
			// TODO(KHJ): 일반 파일 열기 로직 구현
		}

		ImGui::Separator();

		if (ImGui::MenuItem("종료", "Alt+F4"))
		{
			UE_LOG("MainBarWidget: 프로그램 종료 메뉴 선택됨");
			// TODO(KHJ): 프로그램 종료 로직 구현
		}

		ImGui::EndMenu();
	}
}

void UMainBarWidget::OnDeviceLost()
{
	LogoSRV.Reset();
}

void UMainBarWidget::OnDeviceRestored(ID3D11Device* dev)
{
	LoadLogo(dev);
}

/**
 * @brief Windows 토글 메뉴를 렌더링하는 함수
 * 등록된 MainMenu를 제외한 모든 UIWindow의 토글 옵션을 표시
 */
void UMainBarWidget::RenderWindowsMenu() const
{
	if (ImGui::BeginMenu("창"))
	{
		if (!UIManager)
		{
			ImGui::Text("UIManager를 사용할 수 없습니다");
			ImGui::EndMenu();
			return;
		}
		
		// 모든 등록된 UIWindow에 대해 토글 메뉴 항목 생성
		const auto& AllWindows = UIManager->GetAllUIWindows();
		
		if (AllWindows.empty())
		{
			ImGui::Text("등록된 창이 없습니다");
		}
		else
		{
			for (auto Window : AllWindows)
			{
				if (!Window)
				{
					continue;
				}
		
				if (Window->GetWindowTitle() == "MainMenuBar")
				{
					continue;
				}
		
				if (ImGui::MenuItem(Window->GetWindowTitle().data(), nullptr, Window->IsVisible()))
				{
					Window->ToggleVisibility();
		
					// Outliner나 Details 패널인 경우 레이아웃 재정리 요청
					const FString& Title = Window->GetWindowTitle();
					if (Title == "Outliner" || Title == "Details")
					{
						UIManager->OnPanelVisibilityChanged();
					}
		
					UE_LOG("MainBarWidget: %s 창 토글됨 (현재 상태: %s)",
						Window->GetWindowTitle().data(),
						Window->IsVisible() ? "표시" : "숨김");
				}
			}
		
			ImGui::Separator();
		
			// 전체 창 제어 옵션
			if (ImGui::MenuItem("모든 창 표시"))
			{
				UIManager->ShowAllWindows();
				UE_LOG("MainBarWidget: 모든 창 표시됨");
			}
		
			if (ImGui::MenuItem("모든 창 숨김"))
			{
				for (auto Window : UIManager->GetAllUIWindows())
				{
					if (!Window)
					{
						continue;
					}
		
					if (Window->GetWindowTitle() == "MainMenuBar")
					{
						continue;
					}
		
		
					Window->SetWindowState(EUIWindowState::Hidden);
				}
		
				UE_LOG("MainBarWidget: 모든 창 숨겨짐");
			}
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief View 메뉴를 렌더링하는 함수
 * ViewMode 선택 기능 (Lit, Unlit, Wireframe)
 */
void UMainBarWidget::RenderViewMenu()
{
	if (ImGui::BeginMenu("보기"))
	{
		// LevelManager에서 Editor 가져오기
		//ULevelManager& LevelMgr = ULevelManager::GetInstance();
		//UEditor* EditorInstance = LevelMgr.GetEditor();
		//if (!EditorInstance)
		//{
		//	ImGui::Text("에디터를 사용할 수 없습니다");
		//	ImGui::EndMenu();
		//	return;
		//}
		//
		//EViewModeIndex CurrentMode = EditorInstance->GetViewMode();
		//
		//// ViewMode 메뉴 아이템
		//bool bIsLit = (CurrentMode == EViewModeIndex::VMI_Lit);
		//bool bIsUnlit = (CurrentMode == EViewModeIndex::VMI_Unlit);
		//bool bIsWireframe = (CurrentMode == EViewModeIndex::VMI_Wireframe);
		//
		//if (ImGui::MenuItem("조명 적용(Lit)", nullptr, bIsLit) && !bIsLit)
		//{
		//	EditorInstance->SetViewMode(EViewModeIndex::VMI_Lit);
		//	UE_LOG("MainBarWidget: ViewMode를 Lit으로 변경");
		//}
		//
		//if (ImGui::MenuItem("조명 비적용(Unlit)", nullptr, bIsUnlit) && !bIsUnlit)
		//{
		//	EditorInstance->SetViewMode(EViewModeIndex::VMI_Unlit);
		//	UE_LOG("MainBarWidget: ViewMode를 Unlit으로 변경");
		//}
		//
		//if (ImGui::MenuItem("와이어프레임(Wireframe)", nullptr, bIsWireframe) && !bIsWireframe)
		//{
		//	EditorInstance->SetViewMode(EViewModeIndex::VMI_Wireframe);
		//	UE_LOG("MainBarWidget: ViewMode를 Wireframe으로 변경");
		//}

		ImGui::EndMenu();
	}
}

/**
 * @brief ShowFlags 메뉴를 렌더링하는 함수
 * Static Mesh, BillBoard 등의 플래그 상태 확인 및 토글
 */
void UMainBarWidget::RenderShowFlagsMenu()
{
	if (ImGui::BeginMenu("표시 옵션"))
	{

		ULevel* CurrentLevel = GWorld->GetCurrentLevel();
		if (!CurrentLevel)
		{
			ImGui::Text("현재 레벨을 찾을 수 없습니다");
			ImGui::EndMenu();
			return;
		}
		
		// ShowFlags 가져오기
		uint64 ShowFlags = CurrentLevel->GetShowFlags();
		
		// Primitives 표시 옵션
		bool bShowPrimitives = (ShowFlags & EEngineShowFlags::SF_Primitives) != 0;
		if (ImGui::MenuItem("프리미티브 표시", nullptr, bShowPrimitives))
		{
			if (bShowPrimitives)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_Primitives);
				UE_LOG("MainBarWidget: Primitives 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_Primitives);
				UE_LOG("MainBarWidget: Primitives 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
		}
		
		// BillBoard Text 표시 옵션
		bool bShowBillboardText = (ShowFlags & EEngineShowFlags::SF_BillboardText) != 0;
		if (ImGui::MenuItem("빌보드 표시", nullptr, bShowBillboardText))
		{
			if (bShowBillboardText)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
				UE_LOG("MainBarWidget: 빌보드 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
				UE_LOG("MainBarWidget: 빌보드 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
		}
		
		// Bounds 표시 옵션
		bool bShowBounds = (ShowFlags & EEngineShowFlags::SF_Bounds) != 0;
		if (ImGui::MenuItem("바운딩박스 표시", nullptr, bShowBounds))
		{
			if (bShowBounds)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_Bounds);
				UE_LOG("MainBarWidget: 바운딩박스 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_Bounds);
				UE_LOG("MainBarWidget: 바운딩박스 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief Help 메뉴에 대한 렌더링 함수
 */
void UMainBarWidget::RenderHelpMenu()
{
	if (ImGui::BeginMenu("도움말"))
	{
		if (ImGui::MenuItem("정보", "F1"))
		{
			UE_LOG("MainBarWidget: 정보 메뉴 선택됨");
			// TODO(KHJ): 정보 다이얼로그 표시
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief 레벨 저장 기능
 */
void UMainBarWidget::SaveCurrentLevel()
{
	path FilePath = OpenSaveFileDialog();
	if (!FilePath.empty())
	{
		//ULevelManager& LevelManager = ULevelManager::GetInstance();
		//
		//try
		//{
		//	bool bSuccess = LevelManager.SaveCurrentLevel(FilePath.string());
		//
		//	if (bSuccess)
		//	{
		//		UE_LOG("MainMenuBar: SceneIO: Level Saved Successfully");
		//	}
		//	else
		//	{
		//		UE_LOG("MainMenuBar: SceneIO: Failed To Save Level");
		//	}
		//}
		//catch (const exception& Exception)
		//{
		//	FString StatusMessage = FString("Save Error: ") + Exception.what();
		//	UE_LOG("MainMenuBar: SceneIO: Save Error: %s", Exception.what());
		//}
	}
}

/**
 * @brief 레벨 로드 기능
 */
void UMainBarWidget::LoadLevel()
{
	path FilePath = OpenLoadFileDialog();

	if (!FilePath.empty())
	{
		try
		{
			// 파일명에서 확장자를 제외하고 레벨 이름 추출
			//FString LevelName = FilePath.stem().string();
			//
			//ULevelManager& LevelManager = ULevelManager::GetInstance();
			//bool bSuccess = LevelManager.LoadLevel(LevelName, FilePath.string());
			//
			//if (bSuccess)
			//{
			//	UE_LOG("MainMenuBar: SceneIO: 레벨 로드에 성공했습니다");
			//}
			//else
			//{
			//	UE_LOG("MainMenuBar: SceneIO: 레벨 로드에 실패했습니다");
			//}
		}
		catch (const exception& Exception)
		{
			FString StatusMessage = FString("Load Error: ") + Exception.what();
			UE_LOG("SceneIO: Load Error: %s", Exception.what());
		}
	}
}

/**
 * @brief 새로운 레벨 생성 기능
 */
void UMainBarWidget::CreateNewLevel()
{
	//ULevelManager::GetInstance().CreateNewLevel();
}

/**
 * @brief Windows API를 활용한 파일 저장 Dialog Modal을 생성하는 UI Window 기능
 * PWSTR: WideStringPointer 클래스
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UMainBarWidget::OpenSaveFileDialog()
{
	path ResultPath = L"";

	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(ResultHandle))
	{
		//IFileSaveDialog* FileSaveDialogPtr = nullptr;
		//
		//// 2. FileSaveDialog 인스턴스 생성
		//ResultHandle = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
		//	IID_IFileSaveDialog, reinterpret_cast<void**>(&FileSaveDialogPtr));
		//
		//if (SUCCEEDED(ResultHandle))
		//{
		//	// 3. 대화상자 옵션 설정
		//	// 파일 타입 필터 설정
		//	COMDLG_FILTERSPEC SpecificationRange[] = {
		//		{L"JSON Files (*.json)", L"*.json"},
		//		{L"All Files (*.*)", L"*.*"}
		//	};
		//	FileSaveDialogPtr->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);
		//
		//	// 기본 필터를 "JSON Files" 로 설정
		//	FileSaveDialogPtr->SetFileTypeIndex(1);
		//
		//	// 기본 확장자 설정
		//	FileSaveDialogPtr->SetDefaultExtension(L"json");
		//
		//	// 대화상자 제목 설정
		//	FileSaveDialogPtr->SetTitle(L"Save Level File");
		//
		//	// Set Flag
		//	DWORD DoubleWordFlags;
		//	FileSaveDialogPtr->GetOptions(&DoubleWordFlags);
		//	FileSaveDialogPtr->SetOptions(DoubleWordFlags | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);
		//
		//	// Show Modal
		//	// 현재 활성 창을 부모로 가짐
		//	UE_LOG("SceneIO: Save Dialog Modal Opening...");
		//	ResultHandle = FileSaveDialogPtr->Show(GetActiveWindow());
		//
		//	// 결과 처리
		//	// 사용자가 '저장' 을 눌렀을 경우
		//	if (SUCCEEDED(ResultHandle))
		//	{
		//		UE_LOG("SceneIO: Save Dialog Modal Closed - 파일 선택됨");
		//		IShellItem* ShellItemResult;
		//		ResultHandle = FileSaveDialogPtr->GetResult(&ShellItemResult);
		//		if (SUCCEEDED(ResultHandle))
		//		{
		//			// Get File Path from IShellItem
		//			PWSTR FilePath = nullptr;
		//			ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);
		//
		//			if (SUCCEEDED(ResultHandle))
		//			{
		//				ResultPath = path(FilePath);
		//				CoTaskMemFree(FilePath);
		//			}
		//
		//			ShellItemResult->Release();
		//		}
		//	}
		//	// 사용자가 '취소'를 눌렀거나 오류 발생
		//	else
		//	{
		//		UE_LOG("SceneIO: Save Dialog Modal Closed - 취소됨");
		//	}
		//
		//	// Release FileSaveDialog
		//	FileSaveDialogPtr->Release();
		//}
		//
		//// COM 라이브러리 해제
		//CoUninitialize();
	}

	return ResultPath;
}

/**
 * @brief Windows API를 활용한 파일 로드 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UMainBarWidget::OpenLoadFileDialog()
{
	path ResultPath = L"";

	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(ResultHandle))
	{
		//IFileOpenDialog* FileOpenDialog = nullptr;
		//
		//// FileOpenDialog 인스턴스 생성
		//ResultHandle = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
		//	IID_IFileOpenDialog, reinterpret_cast<void**>(&FileOpenDialog));
		//
		//if (SUCCEEDED(ResultHandle))
		//{
		//	// 파일 타입 필터 설정
		//	COMDLG_FILTERSPEC SpecificationRange[] = {
		//		{L"JSON Files (*.json)", L"*.json"},
		//		{L"All Files (*.*)", L"*.*"}
		//	};
		//
		//	FileOpenDialog->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);
		//
		//	// 기본 필터를 "JSON Files" 로 설정
		//	FileOpenDialog->SetFileTypeIndex(1);
		//
		//	// 대화상자 제목 설정
		//	FileOpenDialog->SetTitle(L"Load Level File");
		//
		//	// Flag Setting
		//	DWORD DoubleWordFlags;
		//	FileOpenDialog->GetOptions(&DoubleWordFlags);
		//	FileOpenDialog->SetOptions(DoubleWordFlags | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
		//
		//	// Open Modal
		//	UE_LOG("SceneIO: Load Dialog Modal Opening...");
		//	ResultHandle = FileOpenDialog->Show(GetActiveWindow()); // 현재 활성 창을 부모로
		//
		//	// 결과 처리
		//	// 사용자가 '열기' 를 눌렀을 경우
		//	if (SUCCEEDED(ResultHandle))
		//	{
		//		UE_LOG("SceneIO: Load Dialog Modal Closed - 파일 선택됨");
		//		IShellItem* ShellItemResult;
		//		ResultHandle = FileOpenDialog->GetResult(&ShellItemResult);
		//
		//		if (SUCCEEDED(ResultHandle))
		//		{
		//			// Get File Path from IShellItem
		//			PWSTR ReturnFilePath = nullptr;
		//			ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &ReturnFilePath);
		//
		//			if (SUCCEEDED(ResultHandle))
		//			{
		//				ResultPath = path(ReturnFilePath);
		//				CoTaskMemFree(ReturnFilePath);
		//			}
		//
		//			ShellItemResult->Release();
		//		}
		//	}
		//	// 사용자가 '취소' 를 눌렀거나 오류 발생
		//	else
		//	{
		//		UE_LOG("SceneIO: Load Dialog Modal Closed - 취소됨");
		//	}
		//
		//	FileOpenDialog->Release();
		//}
		//
		//// COM 라이브러리 해제
		//CoUninitialize();
	}

	return ResultPath;
}

void UMainBarWidget::LoadLogo(ID3D11Device* dev)
{
	LogoSRV.Reset();
	HRESULT hr = DirectX::CreateWICTextureFromFileEx(
		dev,
		L"Asset/Logo/EngineCHLogo.png",
		0,                          // maxsize (0 = 원본)
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0, 0,
		DirectX::WIC_LOADER_FORCE_SRGB,   // ★ 핵심: sRGB 강제
		nullptr,
		LogoSRV.GetAddressOf()
	);
}

//void UMainBarWidget::EnsureLogoLoaded()
//{
//	if (LogoTex) return; // 이미 준비됨
//
//	auto* dev = URenderer::GetInstance().GetDevice();
//	if (!dev) return;
//
//	// 필요하면 Ex 버전 + sRGB 플래그 사용 가능
//	HRESULT hr = DirectX::CreateWICTextureFromFile(
//		dev,
//		L"Asset/Logo/EngineCHLogo.png",
//		nullptr,
//		LogoSRV.ReleaseAndGetAddressOf()
//	);
//	if (SUCCEEDED(hr)) {
//		// DX11 백엔드: ImTextureID == ID3D11ShaderResourceView*
//		LogoTex = (ImTextureID)LogoSRV.Get();
//	}
//}
//
//
//void UMainBarWidget::DrawLogoInMenuBar() {
//	EnsureLogoLoaded();
//	const float h = ImGui::GetFrameHeight(); // 메뉴바 높이 = 아이콘 정사각형
//	ImGui::SetCursorPosX(6.0f);              // 왼쪽 여백
//	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
//	if (LogoSRV) ImGui::Image((ImTextureID)LogoSRV.Get(), ImVec2(h, h));
//	else         ImGui::TextUnformatted("UE");
//	ImGui::PopStyleVar();
//	ImGui::SameLine();
//}
