#include "pch.h"
#include "Render/UI/ImGui/ImGuiHelper.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Manager/Input/InputManager.h"
#include "Manager/Time/TimeManager.h"

#include "Render/Renderer/Renderer.h"

// 테스트용 Camera
#include "Editor/Camera.h"
#include "Manager/Path/PathManager.h"

IMPLEMENT_CLASS(UImGuiHelper, UObject);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

UImGuiHelper::UImGuiHelper() = default;

UImGuiHelper::~UImGuiHelper() = default;

/**
 * @brief ImGui 초기화 함수
 */
void UImGuiHelper::Initialize(HWND InWindowHandle)
{
	if (bIsInitialized)
	{
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(InWindowHandle);

	ImGuiIO& IO = ImGui::GetIO();
	ImGuiStyle& Style = ImGui::GetStyle();
	//Style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
	path FontFilePath = UPathManager::GetInstance().GetFontPath() / "Pretendard-Regular.otf";

	// 1. C++의 파일 스트림을 사용하여 폰트 파일을 바이너리 모드로 엽니다.
	//    std::filesystem::path는 Windows에서 유니코드 경로를 자동으로 올바르게 처리합니다.
	std::ifstream font_file(FontFilePath, std::ios::binary | std::ios::ate);

	if (font_file.is_open())
	{
		// 2. 파일 크기를 확인하고, 그 크기만큼의 메모리 버퍼(vector)를 준비합니다.
		std::streamsize size = font_file.tellg();
		font_file.seekg(0, std::ios::beg);

		std::vector<char> font_buffer(size);

		// 3. 파일 내용을 메모리 버퍼로 한 번에 읽어들입니다.
		if (font_file.read(font_buffer.data(), size))
		{
			// 4. ImGui에 파일 경로가 아닌, 메모리 버퍼를 직접 전달합니다.
			ImFontConfig font_cfg;
			// [중요] 폰트 데이터의 소유권이 ImGui가 아닌 우리 코드(font_buffer)에 있음을 알려줍니다.
			// 이렇게 하지 않으면 ImGui가 종료될 때 이 메모리를 해제하려고 시도하여 크래시가 발생할 수 있습니다.
			font_cfg.FontDataOwnedByAtlas = false;

			IO.Fonts->AddFontFromMemoryTTF(
				font_buffer.data(),      // 폰트 데이터가 담긴 메모리 포인터
				font_buffer.size(),      // 데이터 크기 (바이트)
				16.0f,                   // 폰트 크기
				&font_cfg,               // 폰트 설정
				IO.Fonts->GetGlyphRangesKorean()
			);
		}
		font_file.close();
	}

	/*ImGuiIO& IO = ImGui::GetIO();
	path FontFilePath = UPathManager::GetInstance().GetFontPath() / "Pretendard-Regular.otf";
	IO.Fonts->AddFontFromFileTTF(FontFilePath.u8string().c_str(), 16.0f, nullptr, IO.Fonts->GetGlyphRangesKorean());*/

	auto& Renderer = URenderer::GetInstance();
	ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

	bIsInitialized = true;
}

/**
 * @brief ImGui 자원 해제 함수
 */
void UImGuiHelper::Release()
{
	if (!bIsInitialized)
	{
		return;
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	bIsInitialized = false;
}

/**
 * @brief ImGui 새 프레임 시작
 */
void UImGuiHelper::BeginFrame() const
{
	if (!bIsInitialized)
	{
		return;
	}

	// Get New Frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

/**
 * @brief ImGui 렌더링 종료 및 출력
 */
void UImGuiHelper::EndFrame() const
{
	if (!bIsInitialized)
	{
		return;
	}

	// Render ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief WndProc Handler 래핑 함수
 * @return ImGui 자체 함수 반환
 */
LRESULT UImGuiHelper::WndProcHandler(HWND hWnd, uint32 msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
