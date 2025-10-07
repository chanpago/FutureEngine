#pragma once

class UDeviceResources
{
public:
	UDeviceResources(HWND InWindowHandle);
	~UDeviceResources();

	void Create(HWND InWindowHandle);
	void Release();

	void CreateDeviceAndSwapChain(HWND InWindowHandle);
    void CreateIdBuffer();
    void ReleaseIdBuffer();
	void CreateFrameBuffer();
	void CreateDepthBuffer();

	void ReleaseDeviceAndSwapChain();
	void ReleaseDepthBuffer();
	void ReleaseFrameBuffer();

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	IDXGISwapChain* GetSwapChain() const { return SwapChain; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return FrameBufferRTV; }
	ID3D11RenderTargetView* GetIdBufferRTV() const { return IdBufferRTV; }
	ID3D11Texture2D* GetIdBuffer() const { return IdBuffer; }
	ID3D11Texture2D* GetIdStagingBuffer() const { return IdStagingBuffer; }
	ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView; }
	const D3D11_VIEWPORT& GetViewportInfo() const { return ViewportInfo; }
	void UpdateViewport();

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;
	ID3D11Texture2D* IdBuffer = nullptr;
	ID3D11Texture2D* IdStagingBuffer = nullptr;
	ID3D11RenderTargetView* IdBufferRTV = nullptr;

	ID3D11Texture2D* DepthBuffer = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;

	D3D11_VIEWPORT ViewportInfo = {};

	uint32 Width = 0;
	uint32 Height = 0;
};
