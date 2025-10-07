#include "pch.h"
#include "Manager/Overlay/OverlayManager.h"
#include "Render/Renderer/Renderer.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

IMPLEMENT_CLASS(UOverlayManager, UObject)
IMPLEMENT_SINGLETON(UOverlayManager)

static constexpr float OVERLAY_MARGIN_X = 20.0f;
static constexpr float OVERLAY_MARGIN_Y = 60.0f; 
static constexpr float OVERLAY_LINE_HEIGHT = 25.0f;
 
UOverlayManager::UOverlayManager() = default; 
UOverlayManager::~UOverlayManager() = default;

TArray<FString> TypeNames = { "None", "FPS", "Memory", "All" };

void UOverlayManager::Release()
{
	ReleaseDirect2D();
	ReleaseDirectWrite();
}
void UOverlayManager::RenderOverlay()
{	
	if (CurrentOverlayType == EOverlayType::NONE)
	{
		return;
	}
	
	//  처음 렌더링할 때, Direct2D 초기화 수행
	if (!D2DRenderTarget)
	{
		static bool bInit = false;
		if (!bInit)
		{
			bInit = true;
			// Direct2D 초기화 시도
			if (!InitDirect2D())
			{
				UE_LOG("InitDirect2D 실패");
				return;
			}
	
			// DirectWrite 초기화 시도
			if (!InitDirectWrite())
			{
				UE_LOG("InitDirectWrite 실패");
				return;
			}
	
			UE_LOG("Direct2D 지연 초기화 성공");
		}
		  
	}
	
	//// 여전히 D2DRenderTarget이 null이면 초기화 실패
	if (!D2DRenderTarget)
	{
		UE_LOG("D2DRenderTarget 초기화 실패");
		return;
	}
	
	D2DRenderTarget->BeginDraw();
	
	switch (CurrentOverlayType)
	{
	case EOverlayType::FPS:
		RenderFPSOverlay();
		break;
	
	case EOverlayType::Memory:
		//RenderMemoryOverlay();
		UE_LOG("Have to Add Memory Layout");
		break;
	
	case EOverlayType::All:
		RenderFPSOverlay();
		UE_LOG("Have to Add Memory Layout");
		//RenderMemoryOverlay();
		break;
	
	default:
		break;
	}
	
	HRESULT ResultHandle = D2DRenderTarget->EndDraw();
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: Direct2D 렌더링 실패: 0x%08lX", ResultHandle);
		if (ResultHandle == D2DERR_RECREATE_TARGET)
		{
			UE_LOG("OverlayManagerSubsystem: 렌더 타겟 재생성 필요");
		}
	}
	else
	{
		// if (renderCount <= 5)
		// {
		// 	UE_LOG("OverlayManagerSubsystem: 렌더링 성공");
		// }
	}

}

void UOverlayManager::SetOverlayType(EOverlayType InType)
{
	CurrentOverlayType = InType;

	UE_LOG("OverlayManagerSubsystem: 오버레이 타입 변경: %s", TypeNames[static_cast<int>(InType)].data());
}

void UOverlayManager::UpdateFPS()
{
	if (DT > 0.0f)
	{
		// FPS 샘플 배열에 추가
		FrameSpeedSamples[FrameSpeedSampleIndex] = 1.0f / DT;
		FrameSpeedSampleIndex = (FrameSpeedSampleIndex + 1) % FPS_SAMPLE_COUNT;
		TotalTime += DT;
	}

	// 평균 FPS 계산
	float FPSSum = 0.0f;
	int32 ValidSampleCount = 0;
	
	for (int32 i = 0; i < FPS_SAMPLE_COUNT; ++i)
	{
		if (FrameSpeedSamples[i] > 0.0f)
		{
			FPSSum += FrameSpeedSamples[i];
			++ValidSampleCount;
		}
	}
	
	if (ValidSampleCount > 0)
	{
		CurrentFPS = FPSSum / ValidSampleCount;
	}
}

void UOverlayManager::ReleaseBackBufferResources()
{
    // Release D2D resources that directly reference the swap-chain backbuffer
    if (TextBrush)
    {
        TextBrush->Release();
        TextBrush = nullptr;
    }
    if (D2DRenderTarget)
    {
        D2DRenderTarget->Release();
        D2DRenderTarget = nullptr;
    }
}

void UOverlayManager::RecreateBackBufferResources()
{
    // Ensure we have a D2D factory (create once if missing)
    if (!D2DFactory)
    {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
        if (FAILED(hr))
        {
            UE_LOG("OverlayManagerSubsystem: Failed to (re)create D2DFactory: 0x%08lX", hr);
            return;
        }
    }

    // Rebind render target to current swap-chain backbuffer
    auto& Renderer = URenderer::GetInstance();
    IDXGISwapChain* SwapChain = Renderer.GetSwapChain();
    if (!SwapChain)
    {
        UE_LOG("OverlayManagerSubsystem: SwapChain not available for D2D RT recreation");
        return;
    }

    IDXGISurface* DxgiBackBuffer = nullptr;
    HRESULT ResultHandle = SwapChain->GetBuffer(0, IID_PPV_ARGS(&DxgiBackBuffer));
    if (FAILED(ResultHandle))
    {
        UE_LOG("OverlayManagerSubsystem: GetBuffer failed during RT recreation: 0x%08lX", ResultHandle);
        return;
    }

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f, 96.0f);

    ResultHandle = D2DFactory->CreateDxgiSurfaceRenderTarget(DxgiBackBuffer, &rtProps, &D2DRenderTarget);
    DxgiBackBuffer->Release();
    if (FAILED(ResultHandle))
    {
        UE_LOG("OverlayManagerSubsystem: CreateDxgiSurfaceRenderTarget failed: 0x%08lX", ResultHandle);
        return;
    }

    // Recreate brush if needed (released alongside RT)
    if (!TextBrush)
    {
        ResultHandle = D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &TextBrush);
        if (FAILED(ResultHandle))
        {
            UE_LOG("OverlayManagerSubsystem: TextBrush recreation failed: 0x%08lX", ResultHandle);
            return;
        }
    }

    // Ensure DirectWrite objects exist (not backbuffer-dependent)
    if (!DWriteFactory || !TextFormat)
    {
        // Create DirectWrite once if missing
        if (!DWriteFactory)
        {
            ResultHandle = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&DWriteFactory));
            if (FAILED(ResultHandle))
            {
                UE_LOG("OverlayManagerSubsystem: DWriteFactory recreation failed: 0x%08lX", ResultHandle);
                return;
            }
        }
        if (!TextFormat)
        {
            ResultHandle = DWriteFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                                           DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"ko-kr", &TextFormat);
            if (FAILED(ResultHandle))
            {
                UE_LOG("OverlayManagerSubsystem: TextFormat recreation failed: 0x%08lX", ResultHandle);
                return;
            }
        }
    }
}

bool UOverlayManager::InitDirect2D()
{
	UE_LOG("Direct2D 초기화 시작");

	auto& Renderer = URenderer::GetInstance();

	//Direct2D Factory 생성
	HRESULT ResultHandle = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
	if (FAILED(ResultHandle))
	{
		UE_LOG("Failed to Init Direct2D Factor");
	}

	// Swapchain 가져오기
	IDXGISwapChain* SwapChain = Renderer.GetSwapChain();
	if (!SwapChain)
	{
		UE_LOG("OverlayManagerSubsystem: SwapChain을 찾을 수 없습니다");
		return false;
	}
	
	// DXGI Surface 가져오기
	IDXGISurface* DxgiBackBuffer = nullptr;
	ResultHandle = SwapChain->GetBuffer(0, IID_PPV_ARGS(&DxgiBackBuffer));
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: DXGI 백버퍼 가져오기 실패: 0x%08X", ResultHandle);
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: DXGI 백버퍼 얻기 성공");
	
	//// Direct2D RenderTarget 속성 설정
	D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		96.0f, // dpiX
		96.0f // dpiY
	);
	
	// DXGI Surface와 연결된 Direct2D RenderTarget 생성
	//UE_LOG("OverlayManagerSubsystem: DXGI Surface와 Direct2D RenderTarget 연결 시도 중...");
	ResultHandle = D2DFactory->CreateDxgiSurfaceRenderTarget(
		DxgiBackBuffer,
		&RenderTargetProperties,
		&D2DRenderTarget
	);
	if (FAILED(ResultHandle))
	{
		UE_LOG("Direct2D RenderTarget 생성 실패");
		return false;
	}
	
	// DXGI Surface 해제 (참조 카운트 감소)
	DxgiBackBuffer->Release();
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: DXGI Surface RenderTarget 생성 실패: 0x%08lX", ResultHandle);
		switch (ResultHandle)
		{
		case E_INVALIDARG:
			UE_LOG("OverlayManagerSubsystem: 잘못된 인수 (E_INVALIDARG)");
			break;
		case E_OUTOFMEMORY:
			UE_LOG("OverlayManagerSubsystem: 메모리 부족 (E_OUTOFMEMORY)");
			break;
		case D2DERR_UNSUPPORTED_PIXEL_FORMAT:
			UE_LOG("OverlayManagerSubsystem: 지원되지 않는 픽셀 포맷");
			break;
		default:
			UE_LOG("OverlayManagerSubsystem: 알 수 없는 오류: 0x%08lX", ResultHandle);
			break;
		}
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: DXGI Surface RenderTarget 생성 성공");
	
	// 텍스트 브러시 생성
	UE_LOG("OverlayManagerSubsystem: TextBrush 생성 시도 중...");
	ResultHandle = D2DRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::White),
		&TextBrush
	);
	
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: TextBrush 생성 실패: 0x%08lX", ResultHandle);
		return false;
	}
	
	UE_LOG("OverlayManagerSubsystem: TextBrush 생성 성공 (Brush: 0x%p)", static_cast<void*>(TextBrush));
	UE_LOG("OverlayManagerSubsystem: Direct2D 초기화 완료");
	
	return true;

}

bool UOverlayManager::InitDirectWrite()
{
	UE_LOG("OverlayManagerSubsystem: DirectWrite 초기화 시작");

	// DirectWrite Factory 생성
	HRESULT ResultHandle = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&DWriteFactory)
	);
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: DWriteFactory 생성 실패: 0x%08X", ResultHandle);
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: DWriteFactory 생성 성공");

	// 텍스트 포맷 생성
	UE_LOG("OverlayManagerSubsystem: TextFormat 생성 시도 중...");
	ResultHandle = DWriteFactory->CreateTextFormat(
		L"Arial", // font family
		nullptr, // font collection
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		18.0f, // font size
		L"ko-kr", // locale
		&TextFormat
	);
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: TextFormat 생성 실패: 0x%08lX", ResultHandle);
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: TextFormat 생성 성공");

	return true;
} 

void UOverlayManager::RenderFPSOverlay() const
{
	static int fpsRenderCount = 0;
	if (fpsRenderCount < 3)
	{
		UE_LOG("OverlayManagerSubsystem: FPS 오버레이 렌더링 (TextBrush: 0x%p, TextFormat: 0x%p)", TextBrush, TextFormat);
		fpsRenderCount++;
	}

	if (!TextBrush || !TextFormat)
	{
		if (fpsRenderCount <= 3)
		{
			UE_LOG("OverlayManagerSubsystem: TextBrush 또는 TextFormat이 null입니다");
		}
		return;
	}
	wchar_t FPSInfoText[64];
	(void)swprintf_s(FPSInfoText, L"FPS: %.1f", CurrentFPS);
	wchar_t GameTimeInfoText[64];
	(void)swprintf_s(GameTimeInfoText, L"(%.0fms)", UTimeManager::GetInstance().GetGameTime() * 1000);
	wchar_t PickingTimeText[256];
	(void)swprintf_s(PickingTimeText, L"Picking Time %fms : Num Attemps %d : Accumulated Time %fms", FWindowsPlatformTime::ToMilliseconds(LastPickTime)
	,TotalPickCount, FWindowsPlatformTime::ToMilliseconds(TotalPickTime));


	uint32 RenderColor = GetFPSColor(CurrentFPS);
	DrawText(FPSInfoText, OVERLAY_MARGIN_X, OVERLAY_MARGIN_Y, RenderColor);
	DrawText(GameTimeInfoText, OVERLAY_MARGIN_X+120, OVERLAY_MARGIN_Y, RenderColor);
	DrawText(PickingTimeText, OVERLAY_MARGIN_X, OVERLAY_MARGIN_Y + OVERLAY_LINE_HEIGHT, RenderColor);
}

void UOverlayManager::DrawText(const wchar_t* InText, float InX, float InY, uint32 InColor) const
{
	if (!D2DRenderTarget || !TextBrush || !TextFormat)
		return;

	// 색상 설정
	D2D1_COLOR_F color;
	color.a = ((InColor >> 24) & 0xFF) / 255.0f;
	color.r = ((InColor >> 16) & 0xFF) / 255.0f;
	color.g = ((InColor >> 8) & 0xFF) / 255.0f;
	color.b = (InColor & 0xFF) / 255.0f;

	TextBrush->SetColor(color);

	// 텍스트 렌더링 영역 설정
	D2D1_RECT_F layoutRect = D2D1::RectF(InX, InY, InX + 400.0f, InY + 50.0f);

	D2DRenderTarget->DrawText(
		InText,
		static_cast<UINT32>(wcslen(InText)),
		TextFormat,
		&layoutRect,
		TextBrush
	);
} 

void UOverlayManager::ReleaseDirect2D()
{
	if (TextBrush)
	{
		TextBrush->Release();
		TextBrush = nullptr;
	}

	if (D2DRenderTarget)
	{
		D2DRenderTarget->Release();
		D2DRenderTarget = nullptr;
	}

	if (D2DFactory)
	{
		D2DFactory->Release();
		D2DFactory = nullptr;
	}
}

void UOverlayManager::ReleaseDirectWrite()
{
	if (TextFormat)
	{
		TextFormat->Release();
		TextFormat = nullptr;
	}

	if (DWriteFactory)
	{
		DWriteFactory->Release();
		DWriteFactory = nullptr; 
	}
}

uint32 UOverlayManager::GetFPSColor(float InFPS)
{
	if (InFPS >= 60.0f)
	{
		return 0xFF00FF00; // 녹색 (우수)
	}
	else if (InFPS >= 30.0f)
	{
		return 0xFFFFFF00; // 노란색 (보통)
	}
	else
	{
		return 0xFFFF0000; // 빨간색 (주의)
	}
}

