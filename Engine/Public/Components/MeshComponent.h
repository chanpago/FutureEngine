#pragma once
#include "Components/PrimitiveComponent.h"


class UMeshComponent : public UPrimitiveComponent
{

	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)
public:

	virtual ~UMeshComponent() {};
	//StaticMesh가 구현되면 주석 해제(09/19 13:05)
	//자식 Component들이 본인의 타입에 맞게 알아서 RenderList에 저장하도록 하기 위해 가상함수 선언
	virtual void AddToRenderList(ULevel* Level) = 0;
	virtual bool IsRayCollided(const FRay& WorldRay, float& ShortestDistance) const = 0;
	///////////////////////////////////////////////////


	//PIE
	virtual void DuplicateSubObjects() override;
	virtual UObject* Duplicate() override;
	virtual void CopyShallow(UObject* Src) override;

};
