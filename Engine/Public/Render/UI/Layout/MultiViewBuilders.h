#pragma once
#include "Render/UI/Layout/SplitterV.h"
#include "Render/UI/Layout/SplitterH.h"
#include "Render/UI/Layout/SViewportWindow.h"

// Helper to build a 2x2 multiview layout
//inline SWindow* BuildQuadView(FViewportClient* TL, FViewportClient* BL, FViewportClient* TR, FViewportClient* BR)
//{
//	auto* Root = new SSplitterV(0.5f);
//	auto* Left = new SSplitterH(0.5f);
//	auto* Right = new SSplitterH(0.5f);
//
//	Root->SetChildren(Left, Right);
//	Left->SetChildren(new SViewportWindow(TL), new SViewportWindow(BL));
//	Right->SetChildren(new SViewportWindow(TR), new SViewportWindow(BR));
//	return Root;
//}
