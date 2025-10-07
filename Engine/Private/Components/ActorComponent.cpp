#include "pch.h"
#include "Components/ActorComponent.h"

IMPLEMENT_ABSTRACT_CLASS(UActorComponent, UObject)

UActorComponent::UActorComponent()
{
	ComponentType = EComponentType::Actor;
	bIsComponentTickEnabled = true;  
}

UActorComponent::~UActorComponent()
{
	SetOuter(nullptr);
	Destroy();
}

void UActorComponent::BeginPlay()
{

}

void UActorComponent::TickComponent()
{

}

void UActorComponent::EndPlay()
{

}

void UActorComponent::Destroy()
{
	
}

void UActorComponent::DuplicateSubObjects()
{
}

void UActorComponent::CopyShallow(UObject* Src)
{
}

UObject* UActorComponent::Duplicate()
{
	Super::Duplicate();

    UActorComponent* NewComp = static_cast<UActorComponent*>(GetClass()->CreateDefaultObject());
    if (!NewComp)
    {
        return nullptr;
    }
    NewComp->CopyShallow(this);
    return NewComp;
}
