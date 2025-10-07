#pragma once

#include "Core/Object.h"
#include <d2d1.h>
#include <dwrite.h>

enum class EOverlayType : uint8
{
	NONE = 0,
	FPS = 1,
	Memory =2,
	All 
};


class UOverlayManager : public UObject
{
	DECLARE_CLASS(UOverlayManager, UObject);
	DECLARE_SINGLETON(UOverlayManager);

public:
	uint64 LastPickTime = 0.0f;
	uint64 TotalPickTime = 0.0f;
	int TotalPickCount = 0;

    void Initialize();
 
    void Release();

    void RenderOverlay();

    void SetOverlayType(EOverlayType InType);

    void UpdateFPS();

    // Release only resources that hold references to the swap-chain backbuffer
    // (safe to call before IDXGISwapChain::ResizeBuffers)
    void ReleaseBackBufferResources();

    // Recreate D2D resources that depend on the swap-chain backbuffer
    // (call after swap-chain buffers are resized)
    void RecreateBackBufferResources();

private:
	EOverlayType CurrentOverlayType = EOverlayType::FPS;

	// Direct2D resource
	ID2D1Factory* D2DFactory = nullptr;
	ID2D1RenderTarget* D2DRenderTarget = nullptr;
	ID2D1SolidColorBrush* TextBrush = nullptr;

	// DirectWrite resource
	IDWriteFactory* DWriteFactory = nullptr;
	IDWriteTextFormat* TextFormat = nullptr;

	// 초기화 함수
	bool InitDirect2D();
	bool InitDirectWrite();

	// Render Overlay Property 
	void RenderFPSOverlay() const;
	///void RenderMemoryOverlay() const;
	void DrawText(const wchar_t* InText, float InX, float InY, uint32 InColor = 0xFFFFFFFF) const;

	// FPS variable
	float CurrentFPS = 0.0f;
	float TotalTime = 0.0f;
	

	static constexpr int32 FPS_SAMPLE_COUNT = 60;
	float FrameSpeedSamples[FPS_SAMPLE_COUNT] = {};
	int32 FrameSpeedSampleIndex = 0;

	//Release
	void ReleaseDirect2D();
	void ReleaseDirectWrite();

	static uint32 GetFPSColor(float InFPS);
};
