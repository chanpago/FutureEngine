#include "pch.h"
#include "Editor/ObjectPicker.h"
#include "Editor/Gizmo.h"
#include "Components/PrimitiveComponent.h"
#include "Actor/Actor.h"
#include "Manager/Input/InputManager.h"
#include "Core/AppWindow.h"
#include "ImGui/imgui.h"
#include "Level/Level.h"

IMPLEMENT_CLASS(UObjectPicker, UObject)

UObjectPicker::UObjectPicker() = default;


FRay UObjectPicker::GetModelRay(const FRay& Ray, const UPrimitiveComponent* Primitive)
{
	FMatrix ModelInverse = Primitive->GetWorldTransformMatrixInverse();

	FRay ModelRay;
	ModelRay.Origin = Ray.Origin * ModelInverse;
	ModelRay.Direction = Ray.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

void UObjectPicker::PickGizmo( const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint, const USceneComponent* SelectedComponent)
{
	//Forward, Right, Up순으로 테스트할거임.
	//원기둥 위의 한 점 P, 축 위의 임의의 점 A에(기즈모 포지션) 대해, AP벡터와 축 벡터 V와 피타고라스 정리를 적용해서 점 P의 축부터의 거리 r을 구할 수 있음.
	//r이 원기둥의 반지름과 같다고 방정식을 세운 후 근의공식을 적용해서 충돌여부 파악하고 distance를 구할 수 있음.

	//FVector4 PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
	//dot(PointOnCylinder - GizmoLocation)*Dot(PointOnCylinder - GizmoLocation) - Dot(PointOnCylinder - GizmoLocation, GizmoAxis)^2 = r^2 = radiusOfGizmo
	//이 t에 대한 방정식을 풀어서 근의공식 적용하면 됨.
	
	FVector GizmoLocation = Gizmo.GetGizmoLocation();
    FVector GizmoAxises[3] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };

    // 로컬 기즈모(또는 스케일 모드)는 항상 로컬 축으로 피킹
    if (!Gizmo.IsWorld() || Gizmo.GetGizmoMode() == EGizmoMode::Scale)
    {
        const FQuat q = FMatrix::ToQuat(SelectedComponent->GetWorldTransformMatrix());
        for (int a = 0; a < 3; ++a)
        {
            GizmoAxises[a] = q.RotateVector(GizmoAxises[a]);
        }
    }
	FVector WorldRayOrigin{ WorldRay.Origin.X,WorldRay.Origin.Y ,WorldRay.Origin.Z };
	FVector WorldRayDirection(WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z);
	switch (Gizmo.GetGizmoMode())
	{
	case EGizmoMode::Translate:
	case EGizmoMode::Scale:
	{
		FVector GizmoDistanceVector = WorldRayOrigin - GizmoLocation;
		bool bIsCollide = false;

		float GizmoRadius = Gizmo.GetTranslateRadius();
		float GizmoHeight = Gizmo.GetTranslateHeight();
		float A, B, C; //Ax^2 + Bx + C의 ABC
		float X; //해
		float Det; //판별식
		//0 = forward 1 = Right 2 = UP

		for (int a = 0; a < 3; a++)
		{
			FVector GizmoAxis = GizmoAxises[a];
			A = 1 - static_cast<float>(pow(WorldRay.Direction.Dot3(GizmoAxis), 2));
			B = WorldRay.Direction.Dot3(GizmoDistanceVector) - WorldRay.Direction.Dot3(GizmoAxis) * GizmoDistanceVector.
				Dot(GizmoAxis); //B가 2의 배수이므로 미리 약분
			C = static_cast<float>(GizmoDistanceVector.Dot(GizmoDistanceVector) -
				pow(GizmoDistanceVector.Dot(GizmoAxis), 2)) - GizmoRadius * GizmoRadius;

			Det = B * B - A * C;
			if (Det >= 0) //판별식 0이상 => 근 존재. 높이테스트만 통과하면 충돌
			{
				X = (-B + sqrtf(Det)) / A;
				FVector PointOnCylinder = WorldRayOrigin + WorldRayDirection * X;
				float Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0) //충돌
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;

				}
				X = (-B - sqrtf(Det)) / A;
				PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
				Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0)
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;
				}
				if (bIsCollide)
				{
					switch (a)
					{
					case 0:
						Gizmo.SetGizmoDirection(EGizmoDirection::Forward);
						return;
					case 1:
						Gizmo.SetGizmoDirection(EGizmoDirection::Right);
						return;
					case 2:
						Gizmo.SetGizmoDirection(EGizmoDirection::Up);
						return;
					}
				}
			}
		}
	} break;
	case EGizmoMode::Rotate:
	{
		for (int a = 0;a < 3;a++)
		{
			if (IsRayCollideWithPlane(WorldRay, GizmoLocation, GizmoAxises[a], CollisionPoint))
			{
				FVector RadiusVector = CollisionPoint - GizmoLocation;
				if (Gizmo.IsInRadius(RadiusVector.Length()))
				{
					switch (a)
					{
					case 0:
						Gizmo.SetGizmoDirection(EGizmoDirection::Forward);
						return;
					case 1:
						Gizmo.SetGizmoDirection(EGizmoDirection::Right);
						return;
					case 2:
						Gizmo.SetGizmoDirection(EGizmoDirection::Up);
						return;
					}
				}
			}
		}
	}
	}
	
	Gizmo.SetGizmoDirection(EGizmoDirection::None);
}

bool UObjectPicker::IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane)
{
	FVector WorldRayOrigin{ WorldRay.Origin.X, WorldRay.Origin.Y ,WorldRay.Origin.Z };

	if (abs(WorldRay.Direction.Dot3(Normal)) < 0.02f)
		return false;

	float Distance = (PlanePoint - WorldRayOrigin).Dot(Normal) / WorldRay.Direction.Dot3(Normal);

	if (Distance < 0)
		return false;
	PointOnPlane = WorldRay.Origin + WorldRay.Direction * Distance;


	return true;
}
