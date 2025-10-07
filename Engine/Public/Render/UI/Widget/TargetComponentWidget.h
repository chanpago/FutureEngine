#pragma once
#include "Widget.h"

class UMaterial;
class UTargetComponentWidget : public UWidget
{
	DECLARE_CLASS(UTargetComponentWidget, UWidget)
public:
	void Initialize() override {};
	void Update() override {};
	void RenderWidget() override {};
	//void PostProcess() override;

	void UpdateTransformFromComponent(USceneComponent* Component);
	void ApplyTransformToComponent(USceneComponent* Component) const;
	void RenderComponentWidget(USceneComponent* Component);
	void RenderStaticMeshComponent(UStaticMeshComponent* Component);
	void RenderBillboardComponent(UBillboardComponent* Component);
	void RenderMaterials(UStaticMeshComponent* Component);
	void RenderTextRenderComponent(UTextRenderComponent* Component);

	void UpdateMaterialListCash();
	void UpdateStaticMeshListCash();
	void UpdateSpriteListCash();
	

	

	// Special Member Function
	UTargetComponentWidget();
	~UTargetComponentWidget() override;

private:

	//캐싱용
	TArray<UStaticMesh*> StaticMeshList;
	TArray<UMaterial*> MaterialList;
	TArray<FString> SpritePathList;
	bool bMaterialListDirty = true;
	bool bStaticMeshListDirty = true;
	bool bSpriteListDirty = true;

	FVector EditLocation;
	FVector EditRotation;
	FVector EditScale;
	bool bScaleChanged;
	bool bRotationChanged;
	bool bPositionChanged;
	uint64 LevelMemoryByte;
	uint32 LevelObjectCount;

	ULevel* LastLevel = nullptr;

	char TextRenderBuffer[256] = "";
	bool bNameChanged = false;
};
