#include "pch.h"
#include "Manager/Time/TimeManager.h"

IMPLEMENT_SINGLETON(UTimeManager)

UTimeManager::UTimeManager()
{
	Initialize();
}

UTimeManager::~UTimeManager() = default;

void UTimeManager::Initialize()
{
	CurrentTime = high_resolution_clock::now();
	PrevTime = CurrentTime;
	GameTime = 0.0f;

	DeltaTime = 0.0f;
	FPS = 0.0f;
	FrameSpeedSampleIndex = 0;

	bIsPaused = false;

	for (int i = 0; i < Time::FPS_SAMPLE_COUNT; ++i)
	{
		FrameSpeedSamples[i] = 0.0f;
	}
}

void UTimeManager::Update()
{
	// 현재 시간 업데이트
	PrevTime = CurrentTime;
	CurrentTime = high_resolution_clock::now();

	// DeltaTime 계산 (초 단위)
	auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - PrevTime);
	DeltaTime = Duration.count() / 1000000.0f;

	if (!bIsPaused)
	{
		GameTime += DeltaTime;
	}

	CalculateFPS();
	++FrameCount;
}

void UTimeManager::CalculateFPS()
{
	if (DeltaTime > 0.0f)
	{
		FrameSpeedSamples[FrameSpeedSampleIndex] = 1.0f / DeltaTime;
		FrameSpeedSampleIndex = (FrameSpeedSampleIndex + 1) % Time::FPS_SAMPLE_COUNT;
	}

	float FPSSum = 0.0f;
	int ValidSampleCount = 0;

	for (int i = 0; i < Time::FPS_SAMPLE_COUNT; ++i)
	{
		if (FrameSpeedSamples[i] > 0.0f)
		{
			FPSSum += FrameSpeedSamples[i];
			++ValidSampleCount;
		}
	}

	if (ValidSampleCount > 0)
	{
		FPS = FPSSum / ValidSampleCount;
	}
}
