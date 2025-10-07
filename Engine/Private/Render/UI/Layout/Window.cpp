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
