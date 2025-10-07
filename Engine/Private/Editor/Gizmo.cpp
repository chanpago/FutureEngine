#include "pch.h"
#include "Editor/Gizmo.h"
#include "Editor/ObjectPicker.h"
#include "Render/Renderer/Renderer.h"
#include "Manager/Input/InputManager.h"
#include "Actor/Actor.h"

IMPLEMENT_CLASS(UGizmo, UObject)

UGizmo::UGizmo()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Primitives.resize(3);
	GizmoColor.resize(3);

	/* *
	* @brief 0: Right, 1: Up, 2: Forward
	*/
	GizmoColor[0] = FVector4(1, 0, 0, 1);
	GizmoColor[1] = FVector4(0, 1, 0, 1);
	GizmoColor[2] = FVector4(0, 0, 1, 1);

	/* *
	* @brief Translation Setting
	*/
	const float ScaleT = TranslateCollisionConfig.Scale;
	Primitives[0].Vertexbuffer = ResourceManager.GetVertexBuffer(EPrimitiveType::Arrow);
	Primitives[0].NumVertices = ResourceManager.GetVertexNum(EPrimitiveType::Arrow);
	Primitives[0].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[0].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[0].bShouldAlwaysVisible = true;

	/* *
	* @brief Rotation Setting
	*/
	Primitives[1].Vertexbuffer = ResourceManager.GetVertexBuffer(EPrimitiveType::Ring);
	Primitives[1].NumVertices = ResourceManager.GetVertexNum(EPrimitiveType::Ring);
	Primitives[1].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[1].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[1].bShouldAlwaysVisible = true;

	/* *
	* @brief Scale Setting
	*/
	Primitives[2].Vertexbuffer = ResourceManager.GetVertexBuffer(EPrimitiveType::CubeArrow);
	Primitives[2].NumVertices = ResourceManager.GetVertexNum(EPrimitiveType::CubeArrow);
	Primitives[2].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[2].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[2].bShouldAlwaysVisible = true;

	/* *
	* @brief Render State
	*/
	PipelineDescKey.BlendType = EBlendType::Opaque;
	PipelineDescKey.DepthStencilType = EDepthStencilType::DepthDisable;
	PipelineDescKey.ShaderType = EShaderType::SampleShader;
	PipelineDescKey.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRasterizerKey RasterizerKey;
	RasterizerKey.CullMode = D3D11_CULL_NONE;
	RasterizerKey.FillMode = D3D11_FILL_SOLID;
	PipelineDescKey.RasterizerKey = RasterizerKey;
}

UGizmo::~UGizmo() = default;

void UGizmo::UpdateCollisionScaleForCamera(const FVector& CameraLocation, USceneComponent* InComponent)
{
	if (!InComponent) return;
	float DistanceToCamera = (CameraLocation - InComponent->GetWorldLocation()).Length();
	float Scale = DistanceToCamera * ScaleFactor;
	if (DistanceToCamera < MinScaleFactor)
	{
		Scale = MinScaleFactor * ScaleFactor;
	}
	TranslateCollisionConfig.Scale = Scale;
	RotateCollisionConfig.Scale = Scale;
}

void UGizmo::RenderGizmo(USceneComponent* Component, const FVector& CameraLocation)
{
	TargetComponent = Component;
    if (!Component) { return; }
    float DistanceToCamera = (CameraLocation - Component->GetWorldLocation()).Length();

	URenderer& Renderer = URenderer::GetInstance();
	const int Mode = static_cast<int>(GizmoMode);
    FEditorPrimitive& EditorPrimitive = Primitives[Mode];
	EditorPrimitive.Location = Component->GetWorldLocation();         

    // 최종 회전은 쿼터니언 합성으로 계산 (정규직교 보장)
    FQuat QuatBase = FQuat::Identity;
    if (GizmoMode == EGizmoMode::Scale || !bIsWorld)
    {
        // 로컬 회전 시에도 드래그 중에 기즈모가 함께 회전하도록 현재 Actor 회전을 사용
        QuatBase = FMatrix::ToQuat(Component->GetWorldTransformMatrix());
    }

	float Scale = DistanceToCamera * ScaleFactor;
	if (DistanceToCamera < MinScaleFactor)
	{
		Scale = MinScaleFactor * ScaleFactor;
	}
	TranslateCollisionConfig.Scale = Scale;
	RotateCollisionConfig.Scale = Scale;

	EditorPrimitive.Scale = FVector(Scale, Scale, Scale);

    // 축 정렬용 고정 회전 (기본 메쉬는 Z-Up 가정)
    const FQuat QuatAlignRight   = FQuat::FromEulerXYZ(FVector{ -0.0f,  90.0f,  0.0f });	// Z->X
    const FQuat QuatAlignUp      = FQuat::FromEulerXYZ(FVector{ -90.0f,  0.0f,  0.0f });	// Z->Y
    const FQuat QuatAlignForward = FQuat::Identity;														// Z->Z

    // X (Right)
    {
        const FQuat QuatFinal = QuatBase * QuatAlignRight;
        EditorPrimitive.Rotation = FQuat::ToEulerXYZ(QuatFinal);
		
    }
    EditorPrimitive.Color = ColorFor(EGizmoDirection::Right);
    Renderer.RenderEditorPrimitive(EditorPrimitive, PipelineDescKey);

    // Y (Up)
    {
        const FQuat QuatFinal = QuatBase * QuatAlignUp;
        EditorPrimitive.Rotation = FQuat::ToEulerXYZ(QuatFinal);

    }
    EditorPrimitive.Color = ColorFor(EGizmoDirection::Up);
    Renderer.RenderEditorPrimitive(EditorPrimitive, PipelineDescKey);

    // Z (Forward)
    {
        const FQuat QuatFinal = QuatBase * QuatAlignForward;
        EditorPrimitive.Rotation = FQuat::ToEulerXYZ(QuatFinal);

    }
    EditorPrimitive.Color = ColorFor(EGizmoDirection::Forward);
    Renderer.RenderEditorPrimitive(EditorPrimitive, PipelineDescKey);
}

void UGizmo::ChangeGizmoMode()
{
	switch (GizmoMode)
	{
	case EGizmoMode::Translate:
		GizmoMode = EGizmoMode::Rotate;
		break;
	case EGizmoMode::Rotate:
		GizmoMode = EGizmoMode::Scale;
		break;
	case EGizmoMode::Scale:
		GizmoMode = EGizmoMode::Translate;
		break;
	}
}

//void UGizmo::SetLocation(const FVector& Location)
//{
//	TargetActor->SetActorLocation(Location);
//}

bool UGizmo::IsInRadius(float Radius)
{
	if (Radius >= RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale && Radius <= RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale)
		return true;
	return false;
}

void UGizmo::OnMouseDragStart(FVector& CollisionPoint, USceneComponent* InComponent)
{
    bIsDragging = true;
    DragStartMouseLocation = CollisionPoint;
    DragStartActorLocation = InComponent->GetRelativeLocation();
    DragStartActorRotation = InComponent->GetRelativeRotation();
    DragStartActorRotationQuat = InComponent->GetRelativeRotationQuat();
    DragStartActorScale = InComponent->GetRelativeScale3D();
}

// 하이라이트 색상은 렌더 시점에만 계산 (상태 오염 방지)
FVector4 UGizmo::ColorFor(EGizmoDirection InAxis) const
{
	const int Idx = AxisIndex(InAxis);
	const FVector4& BaseColor = GizmoColor[Idx];
	const bool bIsHighlight = (InAxis == GizmoDirection);

	const FVector4 Paint = bIsHighlight ? FVector4(1,1,0,1) : BaseColor;

	if (bIsDragging)
		return BaseColor;
	else
		return Paint;
}
