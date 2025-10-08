#include "pch.h"
#include "Render/UI/Layout/Window.h"

// No separate implementation yet, provided for future extension.

void SWindow::OnResize(const FRect& InRect)
{
	Rect = InRect;
	LayoutChildren();
}

void SWindow::OnPaint()
{
	// Draw a border for debugging purposes to visualize the window's rect.
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	ImVec2 p0 = ImVec2((float)Rect.X, (float)Rect.Y);
	ImVec2 p1 = ImVec2((float)(Rect.X + Rect.W), (float)(Rect.Y + Rect.H));
	ImU32 borderColor = IM_COL32(255, 0, 0, 255); // Red border
	drawList->AddRect(p0, p1, borderColor);
}

bool SWindow::IsHover(FPoint coord) const
{
	return (coord.X >= Rect.X) && (coord.X < Rect.X + Rect.W) &&
		(coord.Y >= Rect.Y) && (coord.Y < Rect.Y + Rect.H);
}

bool SWindow::OnMouseDown(FPoint Coord, int Button)
{
	return false;
}

bool SWindow::OnMouseUp(FPoint Coord, int Button)
{
	return false;
}

bool SWindow::OnMouseMove(FPoint Coord)
{
	return false;
}

SWindow* SWindow::HitTest(FPoint ScreenCoord)
{
	return IsHover(ScreenCoord) ? this : nullptr;
}
