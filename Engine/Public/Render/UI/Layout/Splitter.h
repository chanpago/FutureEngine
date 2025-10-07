#pragma once
#include "Render/UI/Layout/Window.h"

// Splitter base class. Owns two child windows and divides Rect using SplitRatio.
class SSplitter : public SWindow
{




public:
	enum class EOrientation { Horizontal, Vertical };

	// Config
	EOrientation Orientation = EOrientation::Vertical;
	float Ratio = 0.5f;          // 0..1
	int32 Thickness = 6;         // handle thickness in px
	int32 MinChildSize = 80;     // min child size in px

	// Children (Left/Top and Right/Bottom)
	SWindow* SideLT = nullptr;   // Left or Top
	SWindow* SideRB = nullptr;   // Right or Bottom

	float* SharedRatio = nullptr;

public:
	void SetChildren(SWindow* InLT, SWindow* InRB);


	// 편의 함수
	float GetEffectiveRatio() const
	{
		return SharedRatio ? *SharedRatio : Ratio;
	}
	void SetEffectiveRatio(float InRatio)
	{
		if (SharedRatio) { *SharedRatio = InRatio; }
		else { Ratio = InRatio; }
	}
	void SetSharedRatio(float* InShared)  // 설정용
	{
		SharedRatio = InShared;
		if (SharedRatio) { *SharedRatio = Ratio; } // 초기값 동기화
	}

	// SWindow overrides
	void LayoutChildren() override;
	bool OnMouseDown(FPoint Coord, int Button) override;
	bool OnMouseUp(FPoint Coord, int Button) override;
	bool OnMouseMove(FPoint Coord) override;
	SWindow* HitTest(FPoint ScreenCoord) override;
	void OnPaint() override;

	// Helpers
	bool IsHandleHover(FPoint Coord) const;
	FRect GetHandleRect() const;

private:
	bool bDragging = false;
	bool bCrossDragging = false;

	// RTTI helper
public:
	SSplitter* AsSplitter() override { return this; }
};
