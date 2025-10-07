#include "pch.h"

#include "public/Render/Viewport/Viewport.h"
#include "public/Render/Viewport/ViewportClient.h"

FViewportClient::FViewportClient()
{
	ViewportCamera = NewObject<UCamera>();
}

FViewportClient::~FViewportClient()
{
	SafeDelete(ViewportCamera);
}

void FViewportClient::Tick() const
{
	ViewportCamera->Update();
}

void FViewportClient::Draw(const FViewport* InViewport) const
{
	if (!InViewport) { return; }

	const float Aspect = InViewport->GetAspect();
	ViewportCamera->SetAspect(Aspect);

	if (IsOrtho())
	{
		
		ViewportCamera->UpdateMatrixByOrth();
		
	}
	else
	{
		ViewportCamera->UpdateMatrixByPers();
	}
}
