#pragma once
#include "Widget.h"

class UInputInformationWidget : public UWidget
{
	DECLARE_CLASS(UInputInformationWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void AddKeyToHistory(const FString& InKeyName);
	static void RenderKeyList(const TArray<EKeyInput, std::allocator<EKeyInput>>& InPressedKeys);
	void RenderMouseInfo() const;
	void RenderKeyStatistics();

	// Special Member Function
	UInputInformationWidget();
	~UInputInformationWidget() override;

private:
	// 키 입력 히스토리
	TArray<FString> RecentKeyPresses;

	// 마우스 관련
	FVector LastMousePosition;
	FVector MouseDelta;

	// 키 입력 통계
	TMap<FString, uint32> KeyPressCount;
};
