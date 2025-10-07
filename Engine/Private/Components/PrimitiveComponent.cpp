#include "pch.h"
#include "Components/PrimitiveComponent.h"
#include "Public/Manager/Resource/ResourceManager.h"

#include <algorithm>

IMPLEMENT_ABSTRACT_CLASS(UPrimitiveComponent, USceneComponent)

//IMPLEMENT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
}


void UPrimitiveComponent::DuplicateSubObjects()
{
}

UObject* UPrimitiveComponent::Duplicate()
{
	//Super::Duplicate();

    UPrimitiveComponent* NewComp = static_cast<UPrimitiveComponent*>(GetClass()->CreateDefaultObject());
    if (!NewComp)
    {
        return nullptr;
    }
    NewComp->CopyShallow(this);
    return NewComp;
}

void UPrimitiveComponent::CopyShallow(UObject* Src)
{
	Super::CopyShallow(Src);

    const UPrimitiveComponent* Base = static_cast<const UPrimitiveComponent*>(Src);

    this->SetColor(Base->GetColor());
    this->SetVisibility(Base->IsVisible());
    this->SetOctreeIndex(-1);
}
