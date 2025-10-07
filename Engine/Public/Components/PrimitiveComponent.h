#pragma once
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Public/Manager/Resource/ResourceManager.h"
#include "Math/AABB.h"

class ULevel;
class UPrimitiveComponent : public USceneComponent
{
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

public:
	UPrimitiveComponent();
	virtual ~UPrimitiveComponent() = default;
	virtual bool IsRayCollided(const FRay& WorldRay, float& ShortestDistance) const  = 0;
	virtual FAABB GetWorldBounds() const = 0;

	FVector4 GetColor() const { return Color; }

	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }
	void SetColor(const FVector4& InColor) { Color = InColor; }
	int32 GetOctreeIndex() const { return OctreeIndex; }
	void SetOctreeIndex(int32 Index) { OctreeIndex = Index; }
	bool IsVisible() const { return bVisible; } 
	 
	//StaticMesh가 구현되면 주석 해제(09/19 13:05)
	//자식 Component들이 본인의 타입에 맞게 알아서 RenderList에 저장하도록 하기 위해 가상함수 선언
	virtual void AddToRenderList(ULevel* Level) = 0;
	///////////////////////////////////////////////////
	
	//FAABB GetWorldBounds() const;

	 
	//PIE
	virtual void DuplicateSubObjects() override;
	virtual UObject* Duplicate() override; 
	virtual void CopyShallow(UObject* Src) override;


	float GetDepthKey() { return DepthKey; }
	void SetDepthKey(float Dist) { DepthKey = Dist; }
protected:
	const TArray<FVertex>* Vertices = nullptr;
	FVector4 Color = FVector4{ 0.f,0.f,0.f,0.f };

	bool bVisible = true;
	int32 OctreeIndex = -1;

	//TEST
	float DepthKey;
};
