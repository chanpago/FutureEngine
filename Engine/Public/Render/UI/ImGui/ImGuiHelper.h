#pragma once
#include "Core/Object.h"

/**
 * @brief ImGui 초기화/렌더링/해제를 담당하는 Helper 클래스
 * UIManager에서 사용하는 유틸리티 클래스
 */
class UImGuiHelper : public UObject
{
	DECLARE_CLASS(UImGuiHelper, UObject);

public:
	UImGuiHelper();
	~UImGuiHelper() override;

	void Initialize(HWND InWindowHandle);
	void Release();

	void BeginFrame() const;
	void EndFrame() const;

	static LRESULT WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

private:
	bool bIsInitialized = false;
};
