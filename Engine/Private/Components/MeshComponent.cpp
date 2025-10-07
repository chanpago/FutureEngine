#include "pch.h"
#include "Components/MeshComponent.h"

IMPLEMENT_ABSTRACT_CLASS(UMeshComponent, UPrimitiveComponent)

void UMeshComponent::DuplicateSubObjects()
{
}

UObject* UMeshComponent::Duplicate()
{
	//Super::Duplicate();

	return nullptr;
}

void UMeshComponent::CopyShallow(UObject* Src)
{
	Super::CopyShallow(Src);
	
}
