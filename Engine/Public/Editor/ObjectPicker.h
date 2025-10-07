#pragma once
#include "Editor/Gizmo.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;
struct FRay;

class UObjectPicker : public UObject
{
	DECLARE_CLASS(UObjectPicker, UObject)

public:
	UObjectPicker();
	void SetCamera(UCamera* Camera);
	bool IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane);
	static FRay GetModelRay(const FRay& Ray, const UPrimitiveComponent* Primitive);

	void PickGizmo(const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint, const USceneComponent* SelectedComponent);

private:

	UCamera* Camera;
};
