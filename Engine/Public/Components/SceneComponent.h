#pragma once
#include "Components/ActorComponent.h"
#include "Public/Manager/Resource/ResourceManager.h"
#include "Math/AABB.h"
#include "Global/Quat.h"

class USceneComponent : public UActorComponent
{
	DECLARE_CLASS(USceneComponent, UActorComponent)

public:
	USceneComponent();
	virtual ~USceneComponent();

	void SetParentAttachment(USceneComponent* SceneComponent);
	void RemoveChild(USceneComponent* ChildDeleted);

	void MarkAsDirty();

	void SetRelativeLocation(const FVector& Location);
    void SetRelativeRotation(const FVector& Rotation);
    void SetRelativeRotation(const FQuat& Rotation) { RelativeRotationQuat = Rotation; RelativeRotation = FQuat::ToEulerXYZ(Rotation); MarkAsDirty(); }
	void SetRelativeScale3D(const FVector& Scale);
	void SetUniformScale(bool bIsUniform);

	void NotMove() { bIsMoved = false; }
	bool IsUniformScale() const;
	bool IsMoved() const { return bIsMoved; }

	const FVector& GetRelativeLocation() const;
	const FVector& GetRelativeRotation() const;
	const FQuat&   GetRelativeRotationQuat() const { return RelativeRotationQuat; }
	const FVector& GetRelativeScale3D() const;
	TArray<USceneComponent*>& GetChildComponents()  { return Children; }
	const USceneComponent* GetParentComponent() const {	return ParentAttachment;}

    const FVector& GetWorldLocation() const;

    const FMatrix& GetWorldTransformMatrix() const;
    const FMatrix& GetWorldTransformMatrixInverse() const;
    USceneComponent* GetParentAttachment() const { return ParentAttachment; }

	virtual void DuplicateSubObjects() override;
	virtual UObject* Duplicate() override;
	virtual void CopyShallow(UObject* Src) override;

	void ChildComponentTransformToParent(USceneComponent* Parent);

protected:
	mutable bool bIsTransformDirty = true;
	mutable bool bIsTransformDirtyInverse = true;
private:
	mutable FMatrix WorldTransformMatrix;
	mutable FMatrix WorldTransformMatrixInverse;



	USceneComponent* ParentAttachment = nullptr;
	//OwnedComponent는 빠른 탐색/ 제거/고유성이 보장되야 하므로 TSet이지만
	//Children은 빠른 순회가 목적이므로 TArray
	TArray<USceneComponent*> Children;
	FVector RelativeLocation = FVector{ 0,0,0.f };
	FVector RelativeRotation = FVector{ 0,0,0.f };
	FQuat   RelativeRotationQuat = FQuat::Identity;
	FVector RelativeScale3D = FVector{ 1.0f,1.0f,1.0f };
	bool bIsUniformScale = false;
	bool bIsMoved = false;
	const float MinScale = 0.01f;
};





