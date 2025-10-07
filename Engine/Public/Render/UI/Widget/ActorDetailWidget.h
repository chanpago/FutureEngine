#pragma once
#include "Widget.h"

class AActor;

class UTargetComponentWidget;
class UUIManager;

class UActorDetailWidget : public UWidget
{
public:
	UActorDetailWidget();
	~UActorDetailWidget() override;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void PostProcess() override;

private:
	void RenderActorInfo(AActor* SelectedActor);
	void RenderComponentTree(USceneComponent* CurrentComponent, USceneComponent** SelectedComponent);
	void RenderNameField();

	UTargetComponentWidget* ComponentWidget = nullptr;
	UUIManager& UIManager;
	char ActorNameBuffer[256] = "";
	bool bNameChanged = false;
};
