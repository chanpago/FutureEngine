#pragma once
class UPrimitiveComponent;

struct PrimitiveComponentTrait
{
	static FAABB GetWorldAABB(const UPrimitiveComponent* InPrimitiveComponent);
	static FVector GetPosition(const UPrimitiveComponent* InPrimitiveComponent);
	static int32 GetOctreeIndex(const UPrimitiveComponent* InPrimitiveComponent);
	static void SetOctreeIndex(UPrimitiveComponent* InPrimitiveComponent, int32 Index);
	static bool IsRayCollided(const UPrimitiveComponent* InPrimitiveComponent, const FRay& WorldRay, float& Distance);
};
