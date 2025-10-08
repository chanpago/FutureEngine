#include "pch.h"
#include "Public/Render/UI/Widget/ViewportControlWidget.h"
#include "Public/Manager/Viewport/ViewportManager.h"
#include "Public/Render/Viewport/Viewport.h"
#include "Public/Render/Viewport/ViewportClient.h"
#include "Public/Manager/Level/World.h"

// 정적 멤버 정의
const char* UViewportControlWidget::ViewModeLabels[] = {
	"Lit", "Unlit", "WireFrame"
};

const char* UViewportControlWidget::ViewTypeLabels[] = {
	"Perspective", "OrthoTop", "OrthoBottom", "OrthoLeft", "OrthoRight", "OrthoFront", "OrthoBack"
};

UViewportControlWidget::UViewportControlWidget()
	: UWidget("Viewport Control Widget")
{
}

UViewportControlWidget::~UViewportControlWidget() = default;

void UViewportControlWidget::Initialize()
{
	UE_LOG("ViewportControlWidget: Initialized");
}

void UViewportControlWidget::Update()
{
	// 필요시 업데이트 로직 추가
	
}

void UViewportControlWidget::RenderWidget()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	if (!ViewportManager.GetRoot())
	{
		return;
	}

	// 먼저 스플리터 선 렌더링 (UI 뒤에서)
	//RenderSplitterLines();

	// 싱글 모드에서는 하나만 렌더링
	if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
	{
		int32 ActiveViewportIndex = ViewportManager.GetActiveIndex();
		RenderViewportToolbar(ActiveViewportIndex);
	}
	else
	{
		for (int32 i = -0;i < 4;++i)
		{
			RenderViewportToolbar(i);
		}
	}
}

void UViewportControlWidget::RenderViewportToolbar(int32 ViewportIndex)
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	// 뷰포트 범위가 벗어날 경우 종료
	if (ViewportIndex >= static_cast<int32>(Viewports.size()) || ViewportIndex >= static_cast<int32>(Clients.size()))
	{
		return;
	}

	const FRect& Rect = Viewports[ViewportIndex]->GetRect();
	if (Rect.W <= 0 || Rect.H <= 0)
	{
		return;
	}


	const int32 ToolbarH = 32;
	const ImVec2 Vec1{ static_cast<float>(Rect.X), static_cast<float>(Rect.Y) };
	const ImVec2 Vec2{ static_cast<float>(Rect.X + Rect.W), static_cast<float>(Rect.Y + ToolbarH) };

	// 1) 툴바 배경 그리기
	ImDrawList* DrawLine = ImGui::GetBackgroundDrawList();
	DrawLine->AddRectFilled(Vec1, Vec2, IM_COL32(30, 30, 30, 100));
	DrawLine->AddLine(ImVec2(Vec1.x, Vec2.y), ImVec2(Vec2.x, Vec2.y), IM_COL32(70, 70, 70, 120), 1.0f);

	// 2) 툴바 UI 요소들을 직접 배치 (별도 창 생성하지 않음)
	// UI 요소들을 화면 좌표계에서 직접 배치
	ImGui::SetNextWindowPos(ImVec2(Vec1.x, Vec1.y));
	ImGui::SetNextWindowSize(ImVec2(Vec2.x - Vec1.x, Vec2.y - Vec1.y));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoTitleBar;

	// 스타일 설정
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

	char WinName[64];
	(void)snprintf(WinName, sizeof(WinName), "##ViewportToolbar%d", ViewportIndex);

	ImGui::PushID(ViewportIndex);
	if (ImGui::Begin(WinName, nullptr, flags))
	{
		// ViewMode 콤보박스
		EViewMode CurrentMode = Clients[ViewportIndex]->GetViewMode();
		int32 CurrentModeIndex = static_cast<int32>(CurrentMode);

		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::Combo("##ViewMode", &CurrentModeIndex, ViewModeLabels, IM_ARRAYSIZE(ViewModeLabels)))
		{
			if (CurrentModeIndex >= 0 && CurrentModeIndex < IM_ARRAYSIZE(ViewModeLabels))
			{
				Clients[ViewportIndex]->SetViewMode(static_cast<EViewMode>(CurrentModeIndex));
			}
		}

		// 구분자
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, 10.0f);

		// ViewType 콤보박스
		EViewType CurType = Clients[ViewportIndex]->GetViewType();
		int32 CurrentIdx = static_cast<int32>(CurType);

		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::Combo("##ViewType", &CurrentIdx, ViewTypeLabels, IM_ARRAYSIZE(ViewTypeLabels)))
		{
			if (CurrentIdx >= 0 && CurrentIdx < IM_ARRAYSIZE(ViewTypeLabels))
			{
				EViewType NewType = static_cast<EViewType>(CurrentIdx);
				Clients[ViewportIndex]->SetViewType(NewType);

				// 카메라 바인딩 로직
				//HandleCameraBinding(ViewportIndex, NewType, CurrentIdx);
			}
		}

		// 구분자
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, 10.0f);

		// 카메라 스피드 표시 및 조정
		//RenderCameraSpeedControl(ViewportIndex);

		// 구분자
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, 10.0f);

		// 그리드 사이즈 조정
		//RenderGridSizeControl();

		// 레이아웃 전환 버튼들을 우측 정렬
		{
			constexpr float RightPadding = 6.0f;
			constexpr float BetweenWidth = 24.0f;

			const float ContentRight = ImGui::GetWindowContentRegionMax().x;
			float TargetX = ContentRight - RightPadding - BetweenWidth;
			TargetX = std::max(TargetX, ImGui::GetCursorPosX());

			ImGui::SameLine();
			ImGui::SetCursorPosX(TargetX);
		}


		// 레이아웃 전환 버튼들
		if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
		{
			if (FUEImgui::ToolbarIconButton("LayoutQuad", EUEViewportIcon::Quad, CurrentLayout == ELayout::Quad))
			{
				CurrentLayout = ELayout::Quad;
				//ViewportManager.SetViewportLayout(EViewportLayout::Quad);

				ViewportManager.StartLayoutAnimation(true, ViewportIndex);
			}
		}

		if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
		{
			if (FUEImgui::ToolbarIconButton("LayoutSingle", EUEViewportIcon::Single, CurrentLayout == ELayout::Single))
			{
				CurrentLayout = ELayout::Single;
				//ViewportManager.SetViewportLayout(EViewportLayout::Single);

				// 스플리터 비율을 저장하고 애니메이션 시작: Quad → Single
				ViewportManager.PersistSplitterRatios();
				ViewportManager.StartLayoutAnimation(false, ViewportIndex);
				
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopID();
}



