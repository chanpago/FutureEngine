#include "pch.h"
#include "Components/PrimitiveComponent.h"

FAABB PrimitiveComponentTrait::GetWorldAABB(const UPrimitiveComponent* InPrimitiveComponent)
{
	return InPrimitiveComponent->GetWorldBounds();
}

FVector PrimitiveComponentTrait::GetPosition(const UPrimitiveComponent* InPrimitiveComponent)
{
	return InPrimitiveComponent->GetWorldLocation();
}

int32 PrimitiveComponentTrait::GetOctreeIndex(const UPrimitiveComponent* InPrimitiveComponent)
{
	return InPrimitiveComponent->GetOctreeIndex();
}

void PrimitiveComponentTrait::SetOctreeIndex(UPrimitiveComponent* InPrimitiveComponent, int32 Index)
{
	InPrimitiveComponent->SetOctreeIndex(Index);
}

bool PrimitiveComponentTrait::IsRayCollided(const UPrimitiveComponent* InPrimitiveComponent, const FRay& WorldRay, float& Distance)
{
	return InPrimitiveComponent->IsRayCollided(WorldRay, Distance);
}
