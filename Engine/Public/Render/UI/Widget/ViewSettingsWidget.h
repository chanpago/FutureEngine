#pragma once
#include "Widget.h"

class UGrid;

class UViewSettingsWidget : public UWidget
{
	DECLARE_CLASS(UViewSettingsWidget, UWidget)
public:
	UViewSettingsWidget();
	~UViewSettingsWidget() override;

	// Special Member Function
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	void SetGrid(UGrid* InGrid) { Grid = InGrid; }

private:
	UGrid* Grid;
	int32 ViewModeIndex = 0;
	bool bShowPrimitiveFlags = false;
};

