#pragma once
#include "Render/UI/Widget/Widget.h"
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;


class UUIManager;

UCLASS()
class UMainBarWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UMainBarWidget, UWidget)

public:
	UMainBarWidget();
	~UMainBarWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	float GetMenuBarHeight() const { return MenuBarHeight; }
	bool IsMenuBarVisible() const { return bIsMenuBarVisible; }



	void OnDeviceLost();                     // (렌더러 이벤트에 연결)
	void OnDeviceRestored(ID3D11Device* dev);
private:

	void RenderWindowsMenu() const;

	static void RenderFileMenu();
	static void RenderViewMenu();
	static void RenderShowFlagsMenu();
	static void RenderHelpMenu();

	static void SaveCurrentLevel();
	static void LoadLevel();
	static void CreateNewLevel();

	static path OpenSaveFileDialog();
	static path OpenLoadFileDialog();



	void LoadLogo(ID3D11Device* dev);
private:
	bool bIsMenuBarVisible = true;
	float MenuBarHeight = 20.0f;
	UUIManager* UIManager = nullptr;
	ComPtr<ID3D11ShaderResourceView> LogoSRV;
	ImTextureID LogoTex{};
	bool bLogoLoaded = false;

};

