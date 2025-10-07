#include "pch.h"
#include "Core/Object.h"
#include "Render/UI/Widget/FPSWidget.h"

#include "Manager/Time/TimeManager.h"

constexpr float REFRESH_INTERVAL = 0.1f;

IMPLEMENT_CLASS(UFPSWidget, UWidget)

UFPSWidget::UFPSWidget()
{
}

UFPSWidget::~UFPSWidget() = default;

void UFPSWidget::Initialize()
{
	// 히스토리 초기화
	for (int i = 0; i < 60; ++i)
	{
		FrameTimeHistory[i] = 0.0f;
	}

	FrameTimeIndex = 0;
	AverageFrameTime = 0.0f;
	CurrentFPS = 0.0f;
	MinFPS = 999.0f;
	MaxFPS = 0.0f;
	TotalGameTime = 0.0f;

	UE_LOG("FPSWidget: Successfully Initialized");
}

void UFPSWidget::Update()
{
	CurrentDeltaTime = DT;
	TotalGameTime += DT;

	auto& TimeManager = UTimeManager::GetInstance();
	CurrentFPS = TimeManager.GetFPS();

	// FPS 통계 업데이트
	MaxFPS = max(CurrentFPS, MaxFPS);
	MinFPS = min(CurrentFPS, MinFPS);

	// 프레임 시간 히스토리 업데이트
	FrameTimeHistory[FrameTimeIndex] = DT * 1000.0f;
	FrameTimeIndex = (FrameTimeIndex + 1) % 60;

	// 평균 프레임 시간 계산
	float Total = 0.0f;
	for (int i = 0; i < 60; ++i)
	{
		Total += FrameTimeHistory[i];
	}
	AverageFrameTime = Total / 60.0f;
}

void UFPSWidget::RenderWidget()
{
	// 러프한 일정 간격으로 FPS 및 Delta Time 정보 출력
	if (TotalGameTime - PreviousTime > REFRESH_INTERVAL)
	{
		PrintFPS = CurrentFPS;
		PrintDeltaTime = CurrentDeltaTime * 1000.0f;
		PreviousTime = TotalGameTime;
	}

	ImVec4 FPSColor = GetFPSColor(CurrentFPS);
	ImGui::TextColored(FPSColor, "FPS: %.1f (%.2f ms)", PrintFPS, PrintDeltaTime);

	// Game Time 출력
	ImGui::Text("Game Time: %.1f s", TotalGameTime);
	ImGui::Text("Total Allocation Count: %s", to_string(TotalAllocationCount).c_str());
	ImGui::Text("Total Allocation Memory Byte: %s", to_string(TotalAllocationBytes).c_str());
	ImGui::Text("Total UObject Count: %s", to_string(GUObjectArray.size()).c_str());
	ImGui::Separator();

	ImGui::Checkbox("Show Details", &bShowGraph);

	// Details
	if (bShowGraph)
	{
		ImGui::Text("Frame Time History:");
		ImGui::PlotLines("##FrameTime", FrameTimeHistory, 60, FrameTimeIndex,
		                 ("Average: " + to_string(AverageFrameTime) + " ms").c_str(),
		                 0.0f, 50.0f, ImVec2(0, 80));

		ImGui::Text("Statistics:");
		ImGui::Text("  Min FPS: %.1f", MinFPS);
		ImGui::Text("  Max FPS: %.1f", MaxFPS);
		ImGui::Text("  Average Frame Time: %.2f ms", AverageFrameTime);

		if (ImGui::Button("Reset Statistics"))
		{
			MinFPS = 999.0f;
			MaxFPS = 0.0f;
			for (int i = 0; i < 60; ++i)
			{
				FrameTimeHistory[i] = 0.0f;
			}
		}
	}

	ImGui::Separator();
}

ImVec4 UFPSWidget::GetFPSColor(float InFPS)
{
	if (InFPS >= 60.0f)
	{
		return {0.0f, 1.0f, 0.0f, 1.0f}; // 녹색 (우수)
	}
	else if (InFPS >= 30.0f)
	{
		return {1.0f, 1.0f, 0.0f, 1.0f}; // 노란색 (보통)
	}
	else
	{
		return {1.0f, 0.0f, 0.0f, 1.0f}; // 빨간색 (주의)
	}
}
