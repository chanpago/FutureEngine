#pragma once
#include "Widget.h"

class UCamera;

class UCameraControlWidget : public UWidget
{
	DECLARE_CLASS(UCameraControlWidget, UWidget)

public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SyncFromCamera();
	void PushToCamera();

	// Setter
	void SetCamera(UCamera* InCamera)
	{
		Camera = InCamera;
	}

	// Special Member Function
	UCameraControlWidget();
	~UCameraControlWidget() override;

private:
	UCamera* Camera = nullptr;
	float UiFovY = 80.f;
	float UiNearZ = 0.1f;
	float UiFarZ = 1000.f;
	int   CameraModeIndex = 0;
};
