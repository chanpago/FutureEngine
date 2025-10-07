#pragma once
#include "Widget.h"

class USceneIOWidget : public UWidget
{
	DECLARE_CLASS(USceneIOWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special Member Function
	USceneIOWidget();
	~USceneIOWidget() override;

private:
	// Level Save & Load
	void SaveLevel(const FString& InFilePath);
	void LoadLevel(const FString& InFilePath);
	void CreateNewLevel();
	static path OpenSaveFileDialog();
	static path OpenLoadFileDialog();

	char NewLevelNameBuffer[256] = "Default";
	FString StatusMessage;
	float StatusMessageTimer = 0.0f;

	static constexpr float STATUS_MESSAGE_DURATION = 3.0f;
};
