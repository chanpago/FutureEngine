#pragma once


class SSplitter;

class SWindow
{
public:
	virtual ~SWindow() = default;


	// Geometry
	virtual void OnResize(const FRect& InRect);
	const FRect& GetRect() const { return Rect; }
	virtual void SetRect(const FRect& InRect) { Rect = InRect; }

	// Painting
	virtual void OnPaint();

	bool IsHover(FPoint coord) const;

	// Input (return true if handled)
	virtual bool OnMouseDown(FPoint Coord, int Button);
	virtual bool OnMouseUp(FPoint Coord, int Button);
	virtual bool OnMouseMove(FPoint Coord);

	// Hit testing (default: self/children-less)
	virtual SWindow* HitTest(FPoint ScreenCoord);

\
	// Splitters override to layout children
	virtual void LayoutChildren() {}

	// Lightweight RTTI helper for window hierarchy
	virtual SSplitter* AsSplitter() { return nullptr; }

protected:
	FRect Rect{};
};

inline SSplitter* Cast(SWindow* In)
{
	return In ? In->AsSplitter() : nullptr;
}
