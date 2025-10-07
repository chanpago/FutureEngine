#include "pch.h"
#include "Components/SceneComponent.h"
#include "Public/Manager/Resource/ResourceManager.h"
#include "Actor/Actor.h"

#include <algorithm>

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

USceneComponent::~USceneComponent()
{
}

void USceneComponent::SetParentAttachment(USceneComponent* NewParent)
{
	if (NewParent == this)
	{
		return;
	}

	if (NewParent == ParentAttachment)
	{
		return;
	}

	//부모의 조상중에 내 자식이 있으면 순환참조 -> 스택오버플로우 일어남.
	for (USceneComponent* Ancester = NewParent; Ancester; Ancester = Ancester->ParentAttachment)
	{
		if (NewParent == this) //조상중에 내 자식이 있다면 조상중에 내가 있을 것임.
			return;
	}

	//부모가 될 자격이 있음, 이제 부모를 바꿈.

	if (ParentAttachment) //부모 있었으면 이제 그 부모의 자식이 아님
	{
		ParentAttachment->RemoveChild(this);
	}

	ParentAttachment = NewParent;

	if (NewParent)
	{
		NewParent->Children.push_back(this);
		ChildComponentTransformToParent(ParentAttachment);
	}

	MarkAsDirty();

}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	/*
	*	이전 코드는 인자로 비교하지 않았음.this로 비교했었음.
	*	이 코드가 쓰이는 곳에서는 this로 인자를 받았기 때문에 잘되었던 것.
	*	따라서 다음과 같이 인자로 비교하도록 수정
	*/
	if (!ChildDeleted) return;
	Children.erase(std::remove(Children.begin(), Children.end(), ChildDeleted), Children.end());
}

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;
	bIsMoved = true;

	for (USceneComponent* Child : Children)
	{
		Child->MarkAsDirty();
	}
}

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	if (!(Location == RelativeLocation))
	{
		RelativeLocation = Location;
		MarkAsDirty();
	}
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	if (!(Rotation == RelativeRotation))
	{
		RelativeRotation = Rotation;
		// Keep quaternion in sync with UI degrees
		RelativeRotationQuat = FQuat::FromEulerXYZ(RelativeRotation);
		MarkAsDirty();
	}
}
void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	if (!(Scale == RelativeScale3D))
	{
		FVector ActualScale = Scale;
		ActualScale.X = std::max(ActualScale.X, MinScale);
		ActualScale.Y = std::max(ActualScale.Y, MinScale);
		ActualScale.Z = std::max(ActualScale.Z, MinScale);
		RelativeScale3D = ActualScale;
		MarkAsDirty();
	}
}

void USceneComponent::SetUniformScale(bool bIsUniform)
{
	bIsUniformScale = bIsUniform;
}



bool USceneComponent::IsUniformScale() const
{
	return bIsUniformScale;
}

const FVector& USceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}
const FVector& USceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}
const FVector& USceneComponent::GetRelativeScale3D() const
{
	return RelativeScale3D;
}

const FVector& USceneComponent::GetWorldLocation() const
{
	const FMatrix& WorldMatrix = GetWorldTransformMatrix();
	return FVector(WorldMatrix.Data[3][0], WorldMatrix.Data[3][1], WorldMatrix.Data[3][2]);
}

const FMatrix& USceneComponent::GetWorldTransformMatrix() const
{
    if (bIsTransformDirty)
    {
        // Quaternion-based TRS (row-major): I * S * R * T
        WorldTransformMatrix = FMatrix::GetModelMatrix(RelativeLocation, RelativeRotationQuat, RelativeScale3D);

        for (USceneComponent* Ancester = ParentAttachment; Ancester; Ancester = Ancester->ParentAttachment)
        {
            WorldTransformMatrix *= FMatrix::GetModelMatrix(Ancester->RelativeLocation, Ancester->RelativeRotationQuat, Ancester->RelativeScale3D);
        }

        bIsTransformDirty = false;
    }

	return WorldTransformMatrix;
}

const FMatrix& USceneComponent::GetWorldTransformMatrixInverse() const
{

    if (bIsTransformDirtyInverse)
    {
        WorldTransformMatrixInverse = FMatrix::Identity;
        for (USceneComponent* Ancestor = ParentAttachment; Ancestor; Ancestor = Ancestor->ParentAttachment)
        {
            WorldTransformMatrixInverse = FMatrix::GetModelMatrixInverse(Ancestor->RelativeLocation, Ancestor->RelativeRotationQuat, Ancestor->RelativeScale3D) * WorldTransformMatrixInverse;
        }
        WorldTransformMatrixInverse = WorldTransformMatrixInverse * FMatrix::GetModelMatrixInverse(RelativeLocation, RelativeRotationQuat, RelativeScale3D);

        bIsTransformDirtyInverse = false;
    }

	return WorldTransformMatrixInverse;
}
 
void USceneComponent::DuplicateSubObjects()
{
	
}
UObject* USceneComponent::Duplicate()
{
	Super::Duplicate();

    USceneComponent* NewSceneComp = static_cast<USceneComponent*>(GetClass()->CreateDefaultObject());
    if (!NewSceneComp)
    {
        return nullptr;
    }
    NewSceneComp->CopyShallow(this);
    // Outer/Owner/Name will be assigned by the duplicating Actor
    NewSceneComp->DuplicateSubObjects();
    return NewSceneComp;
}
void USceneComponent::CopyShallow(UObject* Src)
{
	//Super::CopyShallow(Src);

	const USceneComponent* BaseSceneComp = static_cast<const USceneComponent*>(Src); 
	this->SetRelativeLocation(BaseSceneComp->GetRelativeLocation());
	this->SetRelativeRotation(BaseSceneComp->GetRelativeRotation());
	this->SetRelativeScale3D(BaseSceneComp->GetRelativeScale3D()); 
	this->SetUniformScale(BaseSceneComp->IsUniformScale());
}


void USceneComponent::ChildComponentTransformToParent(USceneComponent* Parent)
{
	for (USceneComponent* Child : Parent->GetChildComponents())
	{
		Child->SetRelativeLocation(Parent->GetRelativeLocation() + Child->GetRelativeLocation());
		Child->SetRelativeRotation(Child->GetRelativeRotationQuat() * Parent->GetRelativeRotationQuat());
		Child->SetRelativeScale3D(Child->GetRelativeScale3D() * Parent->GetRelativeScale3D());

		ChildComponentTransformToParent(Child);
	}
}
