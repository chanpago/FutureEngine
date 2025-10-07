#pragma once
#include "Editor/EditorPrimitive.h"
#include "Core/Object.h"
#include "Actor/Actor.h"

struct FQuat;
class UObjectPicker;

enum class EGizmoMode
{
	Translate,
	Rotate,
	Scale
};

enum class EGizmoDirection
{
	Right,
	Up,
	Forward,
	None
};

struct FGizmoTranslationCollisionConfig
{
	FGizmoTranslationCollisionConfig()
		: Radius(0.04f), Height(0.9f), Scale(2.f) {
	}

	float Radius = {};
	float Height = {};
	float Scale = {};
};

struct FGizmoRotateCollisionConfig
{
	FGizmoRotateCollisionConfig()
		: OuterRadius(1.0f), InnerRadius(0.9f), Scale(2.f) {
	}

	float OuterRadius = {1.0f};  // 링 큰 반지름
	float InnerRadius = {0.9f};  // 링 굵기 r
	float Scale = {2.0f};
};

class UGizmo : public UObject
{
	DECLARE_CLASS(UGizmo, UObject)

public:
	UGizmo();
	~UGizmo() override;
	void ChangeGizmoMode();
	void RenderGizmo(USceneComponent* Component, const FVector& CameraLocation);

	/* *
	* @brief Setter
	*/
	void SetLocation(const FVector& Location);
	void SetGizmoDirection(EGizmoDirection Direction) { GizmoDirection = Direction; }
	//void SetActorRotation(const FVector& Rotation) { TargetActor->SetActorRotation(Rotation); }
	//void SetActorRotation(const FQuat& Rotation) { TargetActor->SetActorRotation(Rotation); }
	//void SetActorScale(const FVector& Scale) { TargetActor->SetActorScale3D(Scale); }

	// 로컬 기즈모, 쿼터니언 구현 후 사용
	void SetWorld() { bIsWorld = true; }
	void SetLocal() { bIsWorld = false; }

	/* *
	* @brief Getter
	*/
	const EGizmoDirection GetGizmoDirection() { return GizmoDirection; }
	const FVector& GetGizmoLocation() { return Primitives[(int)GizmoMode].Location; }
	const FVector& GetActorRotation() { return TargetComponent->GetRelativeRotation(); }
	const FQuat& GetActorRotationQuat() { return TargetComponent->GetRelativeRotationQuat(); }
	const FVector& GetActorScale() { return TargetComponent->GetRelativeScale3D(); }
	const FVector& GetDragStartMouseLocation() { return DragStartMouseLocation; }
	const FVector& GetDragStartActorLocation() { return DragStartActorLocation; }
	const FVector& GetDragStartActorRotation() { return DragStartActorRotation; }
	const FQuat& GetDragStartActorRotationQuat() { return DragStartActorRotationQuat; }
	const FVector& GetDragStartActorScale() { return DragStartActorScale; }
	const EGizmoMode GetGizmoMode() { return GizmoMode; }
	const FVector GetGizmoAxis() {
		FVector Axis[3]{ {1,0,0},{0,1,0},{0,0,1} }; return Axis[AxisIndex(GizmoDirection)];
	}

	float GetTranslateRadius() const { return TranslateCollisionConfig.Radius * TranslateCollisionConfig.Scale; }
	float GetTranslateHeight() const { return TranslateCollisionConfig.Height * TranslateCollisionConfig.Scale; }
	float GetRotateOuterRadius() const { return RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale; }
	float GetRotateInnerRadius() const { return RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale; }
	float GetRotateThickness()   const { return std::max(0.001f, RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale); }
	bool IsInRadius(float Radius);

	/* *
	* @brief 마우스 관련
	*/
	void EndDrag() { bIsDragging = false; }
	bool IsDragging() const { return bIsDragging; }

	//로컬 기즈모, 쿼터니언 구현 후 사용
	bool IsWorld() const { return bIsWorld; }
	void OnMouseHovering() {}
	void OnMouseDragStart(FVector& CollisionPoint, USceneComponent* InComponent);
	void OnMouseRelease(EGizmoDirection DirectionReleased) {}

	// Update collision scales based on current camera (used before render-time update)
	void UpdateCollisionScaleForCamera(const FVector& CameraLocation, USceneComponent* InComponent);

	

private:
	static inline int AxisIndex(EGizmoDirection InDirection)
	{
		switch (InDirection)
		{
		case EGizmoDirection::Right:   return 0;
		case EGizmoDirection::Up:      return 1;
		case EGizmoDirection::Forward: return 2;
		default:                       return 2;
		}
	}

	// 렌더 시 하이라이트 색상 계산(상태 오염 방지)
	FVector4 ColorFor(EGizmoDirection InAxis) const;


	TArray<FEditorPrimitive> Primitives;
	USceneComponent* TargetComponent = nullptr;

	TArray<FVector4> GizmoColor;
	FVector DragStartActorLocation;
	FVector DragStartMouseLocation;
	FVector DragStartActorRotation;
	FQuat DragStartActorRotationQuat;
	FVector DragStartActorScale;

	FGizmoTranslationCollisionConfig TranslateCollisionConfig;
	FGizmoRotateCollisionConfig RotateCollisionConfig;
	float HoveringFactor = 0.8f;
	const float ScaleFactor = 0.2f;
	const float MinScaleFactor = 7.0f;
	bool bIsDragging = false;

	//로컬 기즈모. 쿼터니언 구현 후 사용
	bool bIsWorld = true;

	FPipelineDescKey PipelineDescKey;

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
	EGizmoMode      GizmoMode = EGizmoMode::Translate;
};
