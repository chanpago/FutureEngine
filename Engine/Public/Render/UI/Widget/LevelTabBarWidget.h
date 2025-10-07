#pragma once
#include "Render/UI/Widget/Widget.h"
#include <wrl/client.h>   // Microsoft::WRL::ComPtr
#include <d3d11.h>        // ID3D11* 인터페이스들


enum class EPIEUIState
{
	Stopped = 0,
	Playing = 1
};

UCLASS()
class ULevelTabBarWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(ULevelTabBarWidget, UWidget)

public:
	ULevelTabBarWidget();
	~ULevelTabBarWidget();

public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;


	float GetLevelBarHeight()const ;

private:
	void SaveLevel(const FString& InFilePath);
	void LoadLevel(const FString& InFilePath);
	void CreateNewLevel();

	// pie 모드 진입용
	void StartPIE();
	void EndPIE();

	static path OpenSaveFileDialog();
	static path OpenLoadFileDialog();

	void ToolbarVSeparator(float post_spacing = 10.0f, float top_margin_px = 4.0f, float bottom_margin_px = 4.0f, float thickness = 1.0f, ImU32 col = 0, float x_offset = 0.0f);

private:
	UUIManager* UIManager = nullptr;

	// 에디터 좌상단 언리얼 마크
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LeftIconSRV;

	// load, save, new
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadIconSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SaveIconSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> NewIconSRV;

	// CreateActor
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateActorIconSRV;

	// Play Stop
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PlayPIEIconSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> StopPIEIconSRV;

	// pie
	EPIEUIState PIEUIState = EPIEUIState::Stopped;

	char NewLevelNameBuffer[256] = "Default";
	FString StatusMessage;
	float StatusMessageTimer = 0.0f;

	static constexpr float STATUS_MESSAGE_DURATION = 3.0f;

	const float TabStripHeight = 36.0f;
	const float LevelBarHeight = 35.0f;
	const float TotalLevelBarHeight = TabStripHeight + LevelBarHeight;

};

