#include "pch.h"
#include "Render/UI/Layout/Splitter.h"
#include "Editor/Editor.h"
#include "Manager/Level/World.h"
#include "Manager/Viewport/ViewportManager.h"
#include "Manager/Input/InputManager.h"

void SSplitter::SetChildren(SWindow* InLT, SWindow* InRB)
{
	SideLT = InLT;
	SideRB = InRB;
}

FRect SSplitter::GetHandleRect() const
{
	const float R = GetEffectiveRatio(); // ★ 핵심: 공유/개별 비율 통일 진입점

	if (Orientation == EOrientation::Vertical)
	{
		int32 splitX = Rect.X + int32(std::lround(Rect.W * R));
		int32 half = Thickness / 2;
		int32 hx = splitX - half;
		return FRect{ hx, Rect.Y, Thickness, Rect.H };
	}
	else
	{
		int32 splitY = Rect.Y + int32(std::lround(Rect.H * R));
		int32 half = Thickness / 2;
		int32 hy = splitY - half;
		return FRect{ Rect.X, hy, Rect.W, Thickness };
	}
}

bool SSplitter::IsHandleHover(FPoint Coord) const
{
	// Slightly extend the hit area to make grabbing the handle easier
	   // and to be more forgiving at the cross intersection.
	const FRect h = GetHandleRect();
	const int32 extend = 4; // pixels of tolerance on each side
	const int32 x0 = h.X - extend;
	const int32 y0 = h.Y - extend;
	const int32 x1 = h.X + h.W + extend;
	const int32 y1 = h.Y + h.H + extend;
	return (Coord.X >= x0) && (Coord.X < x1) && (Coord.Y >= y0) && (Coord.Y < y1);
}



void SSplitter::LayoutChildren()
{
	if (!SideLT || !SideRB)
		return;

	const FRect h = GetHandleRect();
	if (Orientation == EOrientation::Vertical)
	{
		FRect left{ Rect.X, Rect.Y, max(0L, h.X - Rect.X), Rect.H };
		int32 rightX = h.X + h.W;
		FRect right{ rightX, Rect.Y, max(0L, (Rect.X + Rect.W) - rightX), Rect.H };
		SideLT->OnResize(left);
		SideRB->OnResize(right);
	}
	else
	{
		FRect top{ Rect.X, Rect.Y, Rect.W, max(0L, h.Y - Rect.Y) };
		int32 bottomY = h.Y + h.H;
		FRect bottom{ Rect.X, bottomY, Rect.W, max(0L, (Rect.Y + Rect.H) - bottomY) };
		SideLT->OnResize(top);
		SideRB->OnResize(bottom);
	}

}

bool SSplitter::OnMouseDown(FPoint Coord, int Button)
{
	// 기즈모가 드래그 중이면 스플리터 조작 차단
	if (GWorld)
	{
		if (UEditor* Editor = GWorld->GetOwningEditor())
		{
			if (Editor->GetGizmo() && Editor->GetGizmo()->IsDragging())
			{
				return false;
			}
		}
	}

	if (Button == 0 && IsHandleHover(Coord))
	{
		bDragging = true;
		// Determine if we are dragging at the cross (both handles overlap)
		bCrossDragging = false;
		if (Orientation == EOrientation::Vertical)
		{
			if (SSplitter* HLeft = Cast(SideLT))
			{
				if (HLeft->Orientation == EOrientation::Horizontal && HLeft->IsHandleHover(Coord)) bCrossDragging = true;
			}
			if (!bCrossDragging)
			{
				if (SSplitter* HRight = Cast(SideRB))
				{
					if (HRight->Orientation == EOrientation::Horizontal && HRight->IsHandleHover(Coord)) bCrossDragging = true;
				}
			}
		}
		else // Horizontal
		{
			if (auto* Root = Cast(UViewportManager::GetInstance().GetRoot()))
			{
				if (Root->Orientation == EOrientation::Vertical && Root->IsHandleHover(Coord))
				{
					bCrossDragging = true;
					UE_LOG("bCrossDragging");
				}
			}
		}
		return true; // handled
	}
	return false;
}

bool SSplitter::OnMouseMove(FPoint Coord)
{
	UE_LOG("0");
	if (!bDragging)
	{
		return IsHandleHover(Coord);
	}

	// Cross-drag: update both axes
	if (bCrossDragging)
	{
		UE_LOG("1");
		if (auto* Root = Cast(UViewportManager::GetInstance().GetQuadRoot()))
		{
			// Vertical ratio from root rect
			const int32 spanX = std::max(1L, Root->Rect.W);
			float rX = float(Coord.X - Root->Rect.X) / float(spanX);
			float limitX = float(Root->MinChildSize) / float(spanX);
			Root->SetEffectiveRatio(std::clamp(rX, limitX, 1.0f - limitX));

			// Horizontal ratio from one of the horizontal splitters (they share a ratio)
			SSplitter* H = Cast(Root->SideLT);
			if (!H || H->Orientation != EOrientation::Horizontal)
			{
				H = Cast(Root->SideRB);
			}
			if (H && H->Orientation == EOrientation::Horizontal)
			{
				const int32 spanY = std::max(1L, H->Rect.H);
				float rY = float(Coord.Y - H->Rect.Y) / float(spanY);
				float limitY = float(H->MinChildSize) / float(spanY);
				H->SetEffectiveRatio(std::clamp(rY, limitY, 1.0f - limitY));
			}

			// Re-layout whole tree
			const FRect current = Root->GetRect();
			Root->OnResize(current);
		}
		return true;
	}

	if (Orientation == EOrientation::Vertical)
	{
		//UE_LOG("v");
		const int32 span = std::max(1L, Rect.W);
		float r = float(Coord.X - Rect.X) / float(span);
		float limit = float(MinChildSize) / float(span);
		SetEffectiveRatio(std::clamp(r, limit, 1.0f - limit)); // ★ 핵심
	}
	else
	{
		//UE_LOG("h");
		const int32 span = std::max(1L, Rect.H);
		float r = float(Coord.Y - Rect.Y) / float(span);
		float limit = float(MinChildSize) / float(span);
		SetEffectiveRatio(std::clamp(r, limit, 1.0f - limit)); // ★ 핵심
	}

	// Re-layout entire viewport tree so siblings using shared ratio update too
	if (auto* Root = UViewportManager::GetInstance().GetQuadRoot())
	{
		const FRect current = Root->GetRect();
		Root->OnResize(current);
	}

	return true;
}

bool SSplitter::OnMouseUp(FPoint, int Button)
{
	if (Button == 0 && bDragging)
	{
		bDragging = false;
		bCrossDragging = false;
		return true;
	}
	return false;
}

void SSplitter::OnPaint()
{

	if (SideLT) SideLT->OnPaint();
	if (SideRB) SideRB->OnPaint();
	// Draw handle line for visual feedback (ImGui overlay)
	const FRect h = GetHandleRect();
	//ImDrawList * dl = ImGui::GetBackgroundDrawList();
	//ImVec2 p0{ (float)h.X, (float)h.Y };
	//ImVec2 p1{ (float)(h.X + h.W), (float)(h.Y + h.H) };
	//dl->AddRectFilled(p0, p1, IM_COL32(80, 80, 80, 160));
	auto& InputManager = UInputManager::GetInstance();
	const FVector& mp = InputManager.GetMousePosition();
	FPoint P{ LONG(mp.X), LONG(mp.Y) };
	bool hovered = IsHandleHover(P);

	if (Orientation == EOrientation::Horizontal)
	{
		// 1) If sibling horizontal handle is hovered, mirror-hover this one too
		if (!hovered)
		{
			if (auto* Root = Cast(UViewportManager::GetInstance().GetRoot()))
			{
				if (Root->Orientation == EOrientation::Vertical)
				{
					SSplitter* leftH = Cast(Root->SideLT);
					SSplitter* rightH = Cast(Root->SideRB);
					if (leftH && leftH->Orientation == EOrientation::Horizontal && leftH != this && leftH->IsHandleHover(P))
						hovered = true;
					if (!hovered && rightH && rightH->Orientation == EOrientation::Horizontal && rightH != this && rightH->IsHandleHover(P))
						hovered = true;
				}
			}
		}

		// 2) If vertical handle (root) is hovered and Y matches our handle band, also hover
		if (!hovered)
		{
			if (auto* Root = Cast(UViewportManager::GetInstance().GetRoot()))
			{
				if (Root->Orientation == EOrientation::Vertical && Root->IsHandleHover(P))
				{
					const FRect hh = GetHandleRect();
					if (P.Y >= hh.Y && P.Y < hh.Y + hh.H)
						hovered = true;
				}
			}
		}
	}

	ImDrawList* dl = ImGui::GetBackgroundDrawList();


	//UE_LOG("splitter rect=(%d,%d %dx%d) handle=(%d,%d %dx%d) mouse=(%d,%d) hovered=%d vp=(%.1f,%.1f)", Rect.X,
	//	Rect.Y, Rect.W, Rect.H, h.X, h.Y, h.W, h.H, (int)P.X, (int)P.Y, hovered ? 1 : 0, ImGui::GetMainViewport()->Pos.x,
	//	ImGui::GetMainViewport()->Pos.y);


	// Only highlight the splitter handle, not the full rect
	ImVec2 p0{ (float)h.X, (float)h.Y };
	ImVec2 p1{ (float)(h.X + h.W), (float)(h.Y + h.H) };
	const ImU32 col = hovered ? IM_COL32(255, 255, 255, 200) : IM_COL32(80, 80, 80, 160);
	dl->AddRectFilled(p0, p1, col);
}

SWindow* SSplitter::HitTest(FPoint ScreenCoord)
{
	// 기즈모가 드래그 중이면 스플리터 hit test 차단
	if (GWorld)
	{
		if (UEditor* Editor = GWorld->GetOwningEditor())
		{
			if (Editor->GetGizmo() && Editor->GetGizmo()->IsDragging())
			{
				// 기즈모 드래그 중에는 스플리터 hit test를 바이패스하고 자식에게 위임
				// Handle has priority so splitter can capture drag
				// 기즈모 드래그 중에는 스플리터가 캡처하지 않음
			}
			else if (IsHandleHover(ScreenCoord))
			{
				return this;
			}
		}
	}
	else
	{
		// Editor가 없는 경우 기존 로직 유지
		if (IsHandleHover(ScreenCoord))
			return this;
	}

	// Delegate to children based on rect containment
	if (SideLT)
	{
		const FRect& r = SideLT->GetRect();
		if (ScreenCoord.X >= r.X && ScreenCoord.X < r.X + r.W &&
			ScreenCoord.Y >= r.Y && ScreenCoord.Y < r.Y + r.H)
		{
			if (auto* hit = SideLT->HitTest(ScreenCoord)) return hit;
			return SideLT;
		}
	}
	if (SideRB)
	{
		const FRect& r = SideRB->GetRect();
		if (ScreenCoord.X >= r.X && ScreenCoord.X < r.X + r.W &&
			ScreenCoord.Y >= r.Y && ScreenCoord.Y < r.Y + r.H)
		{
			if (auto* hit = SideRB->HitTest(ScreenCoord)) return hit;
			return SideRB;
		}
	}
	return SWindow::HitTest(ScreenCoord);
}
