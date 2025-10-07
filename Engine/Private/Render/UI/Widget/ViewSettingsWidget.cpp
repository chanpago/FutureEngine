#include "pch.h"
#include "Render/UI/Widget/ViewSettingsWidget.h"
#include "Render/Renderer/Renderer.h"

// Camera Mode
static const char* CameraMode[] = {
	"Lit",
	"Unlit",
	"WireFrame",
};

IMPLEMENT_CLASS(UViewSettingsWidget, UWidget)

UViewSettingsWidget::UViewSettingsWidget() : Grid(nullptr)
{
}

UViewSettingsWidget::~UViewSettingsWidget() = default;

void UViewSettingsWidget::Initialize()
{
}

void UViewSettingsWidget::Update()
{
}

void UViewSettingsWidget::RenderWidget()
{
	ImGui::TextUnformatted("View Settings");
	ImGui::Spacing();

	// 그리드 간격
	float CurrentCellSize = Grid->GetCellSize();
	if (ImGui::DragFloat("그리드 간격", &CurrentCellSize, 0.001f, 10.0f))
	{
		Grid->SetCellSize(CurrentCellSize);	
	}

	if (ImGui::Combo("View Mode", &ViewModeIndex, CameraMode, IM_ARRAYSIZE(CameraMode)))
	{
		if (ViewModeIndex >= 0 && ViewModeIndex < static_cast<int32>(EViewModeIndex::End))
		{
			URenderer::GetInstance().SetViewMode(static_cast<EViewModeIndex>(ViewModeIndex));
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Primitive Render Toggle Button
	if (ImGui::Button("Primitive Render Toggle"))
	{
		bShowPrimitiveFlags = !bShowPrimitiveFlags;
	}

	// Show checkboxes when toggle is active
	if (bShowPrimitiveFlags)
	{
		ImGui::Spacing();
		ImGui::TextUnformatted("Show Flags:");
		ImGui::Indent();

		auto& Renderer = URenderer::GetInstance();
		EEngineShowFlags CurrentFlags = Renderer.GetShowFlags();

		// Primitives checkbox
		bool bShowPrimitives = HasFlag(CurrentFlags, EEngineShowFlags::SF_Primitives);
		if (ImGui::Checkbox("Primitives", &bShowPrimitives))
		{
			Renderer.ToggleShowFlag(EEngineShowFlags::SF_Primitives);
		}

		// Billboard Text checkbox
		bool bShowBillboardText = HasFlag(CurrentFlags, EEngineShowFlags::SF_BillboardText);
		if (ImGui::Checkbox("Billboard Text", &bShowBillboardText))
		{
			Renderer.ToggleShowFlag(EEngineShowFlags::SF_BillboardText);
		}

		// Grid checkbox
		bool bShowGrid = HasFlag(CurrentFlags, EEngineShowFlags::SF_Grid);
		if (ImGui::Checkbox("Grid", &bShowGrid))
		{
			Renderer.ToggleShowFlag(EEngineShowFlags::SF_Grid);
		}

		// Bounds checkbox
		bool bShowBounds = HasFlag(CurrentFlags, EEngineShowFlags::SF_Bounds);
		if (ImGui::Checkbox("Bounding Boxes (Alt + C)", &bShowBounds))
		{
			Renderer.ToggleShowFlag(EEngineShowFlags::SF_Bounds);
		}

		ImGui::Unindent();
	}
}
