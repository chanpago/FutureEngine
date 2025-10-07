#pragma once
#include "Widget.h"

class AActor;
class ULevel;

class UActorListWidget : public UWidget
{
public:
	UActorListWidget();
	~UActorListWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

private:
	void RenderActorList();
	void RenderActorItem(AActor* InActor, int32 InIndex);
	FString GetActorDisplayName(AActor* InActor) const;
	FString GetActorClassName(AActor* InActor) const;

	ULevel* CurrentLevel = nullptr;
	AActor* SelectedActor = nullptr;
	int32 SelectedActorIndex = -1;
};
