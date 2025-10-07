#pragma once
#include "Render/UI/Layout/Splitter.h"

// Horizontal splitter: Top(LT) / Bottom(RB)
class SSplitterH : public SSplitter
{
public:
	SSplitterH() { Orientation = EOrientation::Horizontal; }
};
