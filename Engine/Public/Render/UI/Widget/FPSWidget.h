#pragma once
#include "Widget.h"

/**
 * @brief Frame과 관련된 내용을 제공하는 UI Widget
 */
class UFPSWidget : public UWidget
{
	DECLARE_CLASS(UFPSWidget, UWidget)

public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	static ImVec4 GetFPSColor(float InFPS);

	// Special Member Function
	UFPSWidget();
	~UFPSWidget() override;

private:
	float FrameTimeHistory[60] = {};
	int32 FrameTimeIndex = 0;
	float AverageFrameTime = 0.0f;

	float CurrentFPS = 0.0f;
	float MinFPS = 999.0f;
	float MaxFPS = 0.0f;

	float TotalGameTime = 0.0f;
	float CurrentDeltaTime = 0.0f;

	// 출력을 위한 변수
	float PreviousTime = 0.0f;
	float PrintFPS;
	float PrintDeltaTime;
	bool bShowGraph;
};
