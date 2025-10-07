#include "pch.h"
#include "Editor/Editor.h"

#include "Editor/Camera.h"
#include "Editor/Gizmo.h"
#include "Editor/Grid.h"
#include "Editor/Axis.h"
#include "Editor/ObjectPicker.h"
#include "Render/Renderer/Renderer.h"
#include "Render/Renderer/LineBatchRenderer.h"
#include "Manager/Level/World.h"
#include "Manager/UI/UIManager.h"
#include "Manager/Input/InputManager.h"
#include "Actor/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Level/Level.h"
#include "Render/UI/Widget/CameraControlWidget.h"
#include "Render/UI/Widget/ViewSettingsWidget.h"
#include "Manager/Overlay/OverlayManager.h"
#include "Manager/Viewport/ViewportManager.h"
#include "public/Render/Viewport/ViewportClient.h"

IMPLEMENT_CLASS(UEditor, UObject)

UEditor::UEditor()
{
	int32 ActiveIndex = UViewportManager::GetInstance().GetActiveIndex();
	ActiveViewportIndex = ActiveIndex;
	Camera = UViewportManager::GetInstance().GetClients()[ActiveIndex]->GetCamera();

	// Set Camera to Control Panel
	auto& UIManager = UUIManager::GetInstance();
	UCameraControlWidget* CameraControlWidget =
		Cast<UCameraControlWidget>(UIManager.FindWidget("UCameraControlWidget"));
	CameraControlWidget->SetCamera(Camera);
	UViewSettingsWidget* ViewSettingsWidget =
		Cast<UViewSettingsWidget>(UIManager.FindWidget("UViewSettingsWidget"));
	ViewSettingsWidget->SetGrid(&Grid);

};

UEditor::~UEditor() = default;

void UEditor::Initialize()
{
	if (GWorld)
	{
		GWorld->SetOwningEditor(this);
	}
	GWorld->CreateNewLevel("Default");
}

void UEditor::Shutdown()
{
	// 종료 시 매인 카메라 설정 저장
	Camera->SaveCameraSettings();

	// 렌더러에서 멀티뷰 카메라 설정을 저장할 것이지만,
	// 추가적으로 여기서도 저장하여 안전성 확보
	auto& Renderer = URenderer::GetInstance();
	Renderer.SaveMultiViewCameraSettings();

	if (GWorld)
	{
		GWorld->SetOwningEditor(nullptr);
	}
}
void UEditor::Update()
{
	auto& Renderer = URenderer::GetInstance();
	UViewportManager& ViewportManager = UViewportManager::GetInstance();
	ULevel* Level = GWorld->GetCurrentLevel();

	// 현재 레벨에 카메라 주입 보장
	EnsureLevelHasCamera(Level);
	// 입력 루틴이 카메라 파라미터를 직접 바꾸므로
	ProcessMouseInput(Level);
	ProcessKeyboardInput();


	// 매 업데이트마다 카메라를 업데이트합니다

	int32 ActiveIndex = UViewportManager::GetInstance().GetActiveIndex();
	if (ActiveViewportIndex != ActiveIndex)
	{
		ActiveViewportIndex = ActiveIndex;
		Camera = UViewportManager::GetInstance().GetClients()[ActiveIndex]->GetCamera();
	}

	// Gate main camera input: in quad, if RMB and hovering non-main viewport, disable main camera input

	if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
	{
		const UInputManager& Input = UInputManager::GetInstance();
		const FVector mp = Input.GetMousePosition();
		FRect rect{};
		int hover = Renderer.GetHoveredViewportIndex(mp.X, mp.Y, rect);
		bool enableMain = !(!Camera->IsDragging() && hover > 0 );
		Camera->SetInputEnabled(enableMain);
	}
	else
	{
		Camera->SetInputEnabled(true);
	}

	// 이전 프레임의 카메라 상태 저장
	static FVector PrevCameraLocation = Camera->GetLocation();
	static FVector PrevCameraRotation = Camera->GetRotation();

	Camera->Update();

	// 카메라 상태 변경 감지 및 자동 저장 요청
	FVector CurrentLocation = Camera->GetLocation();
	FVector CurrentRotation = Camera->GetRotation();
	if ((CurrentLocation - PrevCameraLocation).Length() > 0.1f ||
	    (CurrentRotation - PrevCameraRotation).Length() > 0.1f)
	{
		Renderer.RequestCameraSave();
		PrevCameraLocation = CurrentLocation;
		PrevCameraRotation = CurrentRotation;
	}

	for(int i = 0; i < 4; ++i)
	{
		UCamera* Cam = Renderer.GetViewCameraAt(i);
		if (i == 0)
		{
			Camera->GetCameraType() == ECameraType::ECT_Perspective ? Cam->SetCameraType(ECameraType::ECT_Perspective) : Cam->SetCameraType(ECameraType::ECT_Orthographic);
			Cam->SetFovY(Camera->GetFovY());
		}
		Cam->SetNearZ(Camera->GetNearZ());
		Cam->SetFarZ(Camera->GetFarZ());
	}

	Renderer.UpdateConstant(Camera->GetFViewProjConstants());
}

// void UEditor::RenderEditor()
// {
// 	Grid.RenderGrid();
// 	Axis.Render();
// 	Gizmo.RenderGizmo(GWorld->GetCurrentLevel()->GetSelectedActor(), Camera.GetLocation());
// }

const FVector& UEditor::GetCameraLocation()
{
	return Camera->GetLocation();
}

void UEditor::RenderEditorBatched(const FVector& CameraLocation)
{
	if (GWorld->GetWorldType() != EWorldType::Editor) return;

	ULineBatchRenderer& LineBatch = ULineBatchRenderer::GetInstance();

	/** 모든 라인 렌더링을 하나의 배치로 통합 */
	LineBatch.BeginBatch();
	{
		/** Grid 라인들 추가 */
		Grid.AddToLineBatch(LineBatch);

		/** Axis 라인들 추가 */
		Axis.AddToLineBatch(LineBatch);

		/** 선택된 액터의 AABB만 배치 라인으로 추가 (인스턴싱 사용 안 함) */
		ULevel* Level = GWorld->GetCurrentLevel();

		URenderer& Renderer = URenderer::GetInstance();
		// Alt+C 토글과 동일하게 Bounds 플래그가 켜진 경우에만 표시
		if (Renderer.IsShowFlagEnabled(EEngineShowFlags::SF_Bounds))
		{
			AActor* Selected = UUIManager::GetInstance().GetSelectedActor();
			if (Selected)
			{
				// 선택된 액터의 모든 PrimitiveComponent 월드 AABB를 합산
				FAABB UnionBounds;
				for (UActorComponent* Comp : Selected->GetOwnedComponents())
				{
					if (Comp && Comp->IsA(UPrimitiveComponent::StaticClass()))
					{
						UPrimitiveComponent* Prim = static_cast<UPrimitiveComponent*>(Comp);
						if (Prim->IsVisible())
						{
							UnionBounds.AddAABB(Prim->GetWorldBounds());
						}
					}
				}

				if (UnionBounds.IsValid())
				{
					// AABB 12개 모서리를 8개 정점 + 인덱스(라인리스트)로 추가
					const FVector& mn = UnionBounds.Min;
					const FVector& mx = UnionBounds.Max;

					FVector vertsPos[8] = {
						FVector(mn.X, mn.Y, mn.Z), // 0: v000
						FVector(mn.X, mn.Y, mx.Z), // 1: v001
						FVector(mn.X, mx.Y, mn.Z), // 2: v010
						FVector(mn.X, mx.Y, mx.Z), // 3: v011
						FVector(mx.X, mn.Y, mn.Z), // 4: v100
						FVector(mx.X, mn.Y, mx.Z), // 5: v101
						FVector(mx.X, mx.Y, mn.Z), // 6: v110
						FVector(mx.X, mx.Y, mx.Z)  // 7: v111
					};

					const FVector4 col(0, 1, 0, 1); // 녹색
					TArray<FVertex> aabbVerts;
					aabbVerts.Reserve(8);
					for (int i = 0; i < 8; ++i)
					{
						aabbVerts.Add(FVertex(vertsPos[i], col));
					}

					// 12개 모서리: 24개 인덱스 (라인 리스트)
					TArray<uint32> aabbIdx;
					aabbIdx.Reserve(24);
					// Bottom face (z=min)
					aabbIdx.Add(0); aabbIdx.Add(4);
					aabbIdx.Add(4); aabbIdx.Add(6);
					aabbIdx.Add(6); aabbIdx.Add(2);
					aabbIdx.Add(2); aabbIdx.Add(0);
					// Top face (z=max)
					aabbIdx.Add(1); aabbIdx.Add(5);
					aabbIdx.Add(5); aabbIdx.Add(7);
					aabbIdx.Add(7); aabbIdx.Add(3);
					aabbIdx.Add(3); aabbIdx.Add(1);
					// Vertical edges
					aabbIdx.Add(0); aabbIdx.Add(1);
					aabbIdx.Add(4); aabbIdx.Add(5);
					aabbIdx.Add(6); aabbIdx.Add(7);
					aabbIdx.Add(2); aabbIdx.Add(3);

					LineBatch.AddIndexedLines(aabbVerts, aabbIdx);
				}
			}
		}

	}

	/** 1회 드로우콜로 모든 라인 렌더링 */
	LineBatch.FlushBatch();

	/** Gizmo는 별도로 렌더링 (기존 방식 유지) */
	Gizmo.RenderGizmo(UUIManager::GetInstance().GetSelectedComponent(), CameraLocation);
}


void UEditor::ProcessKeyboardInput()
{
	const UInputManager& InputManager = UInputManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();

	// Alt+C로 바운딩 박스 토글
	if (InputManager.IsKeyDown(EKeyInput::Alt) && InputManager.IsKeyPressed(EKeyInput::C))
	{
		Renderer.ToggleShowFlag(EEngineShowFlags::SF_Bounds);
	}
	if (InputManager.IsKeyPressed(EKeyInput::Space))
	{
		Gizmo.ChangeGizmoMode();
	}

	// F2: Toggle viewport layout (Single <-> Quad)
	if (InputManager.IsKeyPressed(EKeyInput::F2))
	{
		//Renderer.ToggleViewportLayout();
	}

	// Gizmo 표시 중 Tab: 월드→로컬 토글 (기본: 토글, 최초 누르면 로컬 보장)
	if (InputManager.IsKeyPressed(EKeyInput::Tab))
	{
		ULevel* Level = GWorld->GetCurrentLevel();
		if (Level && UUIManager::GetInstance().GetSelectedActor())
		{
			if (Gizmo.IsWorld())
			{
				Gizmo.SetLocal();
			}
			else
			{
				Gizmo.SetWorld();
			}
		}
	}
}

void UEditor::ProcessMouseInput(ULevel* InLevel)
{
	const UInputManager& InputManager = UInputManager::GetInstance();
	URenderer& Renderer = URenderer::GetInstance();
	UViewportManager& ViewportManager = UViewportManager::GetInstance();

	// If user is dragging a splitter, suppress picking/gizmo interactions to avoid conflicts
	if (Renderer.IsDraggingSplitter())
	{
		return;
	}
	UUIManager& UIManager = UUIManager::GetInstance();

	static EGizmoDirection PreviousGizmoDirection = EGizmoDirection::None;
	USceneComponent* SelectedComponent = UIManager.GetSelectedComponent();
	AActor* SelectedActor = UIManager.GetSelectedActor();
	float ActorDistance = -1;

	// Single-view must behave exactly like before: use global NDC and main camera
	const bool bIsQuad = (ViewportManager.GetViewportLayout() == EViewportLayout::Quad);
	// Hovered viewport info (valid only in quad)
	FRect HoverRect{0,0,0,0};
	int HoverIndex = -1;
	UCamera* PickCam = Camera;
	FRay WorldRay = {};
	if (!ImGui::GetIO().WantCaptureMouse)
	{
		if (!bIsQuad)
		{
			FVector MousePositionNdc = InputManager.GetMouseNDCPosition();
			// 싱글뷰는 기존대로 전체 화면 NDC를 사용하지만, 멀티뷰 대비 안전을 위해 종횡비와 행렬 업데이트
			if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
			{
				DXGI_SWAP_CHAIN_DESC scd = {};
				URenderer::GetInstance().GetSwapChain()->GetDesc(&scd);
				float vpAspect = (scd.BufferDesc.Height > 0) ? (float)scd.BufferDesc.Width / (float)scd.BufferDesc.Height : 1.0f;
				PickCam->SetAspect(vpAspect);
				if (PickCam->GetCameraType() == ECameraType::ECT_Perspective) PickCam->UpdateMatrixByPers(); else PickCam->UpdateMatrixByOrth();
			}
			WorldRay = Camera->ConvertToWorldRay(MousePositionNdc.X, MousePositionNdc.Y);
		}
		else
		{
			//FVector pos;
			//// Determine hovered viewport and build ray using its camera
			//const FVector MousePosPx = InputManager.GetMousePosition();
			//HoverIndex = Renderer.GetHoveredViewportIndex(MousePosPx.X, MousePosPx.Y, HoverRect);
			//if (HoverIndex < 0)
			//{
			//	HoverIndex = 0;
			//	DXGI_SWAP_CHAIN_DESC scd = {};
			//	Renderer.GetSwapChain()->GetDesc(&scd);
			//	HoverRect = { 0, 0, (LONG)scd.BufferDesc.Width, (LONG)scd.BufferDesc.Height };
			//}
			//// Convert pixel to local NDC of that viewport rect
			//float LocalNdcX = ((MousePosPx.X - HoverRect.X) / HoverRect.W) * 2.0f - 1.0f;
			//float LocalNdcY = 1.0f - ((MousePosPx.Y - HoverRect.Y) / HoverRect.H) * 2.0f;
			//
			//// 항상 뷰포트 전용 카메라를 사용 (TL도 ViewCameras[0])
			//PickCam = Renderer.GetViewCameraAt(HoverIndex);
			//if (!PickCam || Camera.IsDragging())
			//{
			//	PickCam = Renderer.GetViewCameraAt(0);
			//}
			//// 멀티뷰에서는 선택된 뷰포트 타입에 맞춰 카메라 파라미터를 맞춘 뒤 종횡비 반영
			//URenderer::EViewportType VType = URenderer::EViewportType::Perspective;
			//switch (HoverIndex)
			//{
			//case 0: VType = URenderer::EViewportType::Perspective; break; // TL
			//case 1: VType = URenderer::EViewportType::Right; break;       // BL
			//case 2: VType = URenderer::EViewportType::Top; break;         // TR
			//case 3: VType = URenderer::EViewportType::Front; break;       // BR
			//default: break;
			//}
			//switch (VType)
			//{
			//case URenderer::EViewportType::Perspective:
			//	PickCam->SetCameraType(ECameraType::ECT_Perspective);
			//	PickCam->SetLocation(Camera.GetLocation());
			//	PickCam->SetRotation(Camera.GetRotation());
			//	break;
			//default:
			//	for (int Index = 1; Index < 4; Index++)
			//	{
			//		UCamera* PickCam1 = Renderer.GetViewCameraAt(Index);
			//		if (Index == 2)
			//		{
			//			PickCam1->SetCameraType(ECameraType::ECT_Orthographic);
			//			PickCam1->SetNearZ(0.1f); PickCam1->SetFarZ(1000.f);
			//			PickCam1->SetLocation(Pos + FVector(0, 0, CameraDistance));
			//			PickCam1->SetRotation(FVector(90.0f, 0.0f, 0.0f));
			//		}
			//		else if (Index == 1)	//Right
			//		{
			//			PickCam1->SetCameraType(ECameraType::ECT_Orthographic);
			//			PickCam1->SetNearZ(0.1f); PickCam1->SetFarZ(1000.f);
			//			PickCam1->SetLocation(Pos + FVector(0, CameraDistance, 0));
			//			PickCam1->SetRotation(FVector(0.0f, -90.0f, 0.0f));
			//		}
			//		else
			//		{
			//			PickCam1->SetCameraType(ECameraType::ECT_Orthographic);
			//			PickCam1->SetNearZ(0.1f); PickCam1->SetFarZ(1000.f);
			//			PickCam1->SetLocation(Pos + FVector(-CameraDistance, 0, 0));
			//			PickCam1->SetRotation(FVector(0.0f, 0.0f, 0.0f));
			//		}
			//
			//	}
			//}
			//// Hovered viewport interactive controls (orthographic): pan with WASD/Arrows, zoom with wheel
			//if (PickCam->GetCameraType() == ECameraType::ECT_Orthographic)
			//{
			//	const UInputManager& InputForView = UInputManager::GetInstance();
			//	bool rmbView = InputForView.IsKeyDown(EKeyInput::MouseRight);
			//	// Zoom (wheel always active)
			//	float wheel = InputForView.GetMouseWheelDelta();
			//	if (wheel != 0.0f)
			//	{
			//		float width = Renderer.GetOrthoWorldWidthConst();
			//
			//		float scale = (wheel > 0.0f) ? 0.9f : 1.1f;
			//		for (int i = 0; i < (int)std::abs(wheel); ++i) width *= scale;
			//		width = std::max(1.0f, std::min(5000.0f, width));
			//		//for (int Index = 0; Index < 4;Index++)
			//			Renderer.SetOrthoWorldWidthConst(width);
			//		CameraDistance *= scale;
			//	}
			//	// Pan (only when RMB is held over this viewport)
			//	if (rmbView)
			//	{
			//		float PickCameraSpeed = Camera.GetMoveSpeed();
			//		float base = std::max(0.001f, PickCam->GetOrthoWorldWidth() * 0.06f) * PickCameraSpeed * DT;
			//		bool keyW = InputForView.IsKeyDown(EKeyInput::W) || InputForView.IsKeyDown(EKeyInput::Up);
			//		bool keyS = InputForView.IsKeyDown(EKeyInput::S) || InputForView.IsKeyDown(EKeyInput::Down);
			//		bool keyA = InputForView.IsKeyDown(EKeyInput::A) || InputForView.IsKeyDown(EKeyInput::Left);
			//		bool keyD = InputForView.IsKeyDown(EKeyInput::D) || InputForView.IsKeyDown(EKeyInput::Right);
			//		switch (VType)
			//		{
			//		case URenderer::EViewportType::Top:
			//			if (keyW) Pos.X += base; if (keyS) Pos.X -= base; if (keyA) Pos.Y -= base; if (keyD) Pos.Y += base;
			//			break;
			//		case URenderer::EViewportType::Right:
			//			if (keyW) Pos.Z += base; if (keyS) Pos.Z -= base; if (keyA) Pos.X -= base; if (keyD) Pos.X += base;
			//			break;
			//		case URenderer::EViewportType::Front:
			//			if (keyW) Pos.Z += base; if (keyS) Pos.Z -= base; if (keyA) Pos.Y -= base; if (keyD) Pos.Y += base;
			//			break;
			//		default: break;
			//		}
			//		//PickCam->SetLocation(pos);
			//	}
			//}
			//
			//// 선택된 뷰포트 직사각형 기준의 종횡비를 카메라에 적용해 정확한 레이 계산
			//float vpAspect = (HoverRect.H > 0.0f) ? (HoverRect.W / HoverRect.H) : 1.0f;
			//PickCam->SetAspect(vpAspect);
			//if (PickCam->GetCameraType() == ECameraType::ECT_Perspective) PickCam->UpdateMatrixByPers(); else PickCam->UpdateMatrixByOrth();
			//// 월드 레이
			//WorldRay = PickCam->ConvertToWorldRay(LocalNdcX, LocalNdcY);
			//{
			//	Gizmo.UpdateCollisionScaleForCamera(PickCam->GetLocation(), SelectedComponent);
			//}
			//// 월드 레이
			//WorldRay = PickCam->ConvertToWorldRay(LocalNdcX, LocalNdcY);
		}
	}

	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		// 회전 모드에서 릴리즈 시 마지막 각도 커밋 (로컬/월드 동일)
		if (Gizmo.IsDragging() && Gizmo.GetGizmoMode() == EGizmoMode::Rotate)
		{
			FQuat FinalQuat = GetGizmoDragRotationQuat(WorldRay, *PickCam, SelectedComponent);
			SelectedComponent->SetRelativeRotation(FinalQuat);
		}
		Gizmo.EndDrag();
	}

	if (Gizmo.IsDragging())
	{
		switch (Gizmo.GetGizmoMode())
		{
		case EGizmoMode::Translate:
			{
				FVector GizmoDragLocation = GetGizmoDragLocation(WorldRay, *PickCam, SelectedComponent);
				SelectedComponent->SetRelativeLocation(GizmoDragLocation);
				break;
			}
		case EGizmoMode::Rotate:
			{
				FQuat GizmoDragRotation = GetGizmoDragRotationQuat(WorldRay, *PickCam, SelectedComponent);
				SelectedComponent->SetRelativeRotation(GizmoDragRotation);
				break;
			}
		case EGizmoMode::Scale:
			{
				FVector GizmoDragScale = GetGizmoDragScale(WorldRay, *PickCam, SelectedComponent);
				SelectedComponent->SetRelativeScale3D(GizmoDragScale);
			}
		}
	}
	else
	{
		FVector CollisionPoint;
		/** 기즈모가 출력되고있음. 레이캐스팅을 계속 해야함. */
		if (SelectedComponent && !ImGui::GetIO().WantCaptureMouse)
		{
			ObjectPicker.PickGizmo(WorldRay, Gizmo, CollisionPoint, SelectedComponent);
		}
		else
		{
			Gizmo.SetGizmoDirection(EGizmoDirection::None);
		}
		// 뷰포트 위에 마우스가 있을 때는 ImGui 캡처 여부와 관계 없이 피킹 허용
		/*bool bAllowPicking = true;
		if (bIsQuad)
		{
			bAllowPicking = (HoverIndex >= 0);
		}*/
		/** 기즈모에 호버링되거나 클릭되지 않았을 때. Actor 업데이트해줌. */
		if (Gizmo.GetGizmoDirection() == EGizmoDirection::None)
		{
			//UUIManager::GetInstance().SetSelectedActor(UIManager.GetSelectedActor());
			if (PreviousGizmoDirection != EGizmoDirection::None)
			{
				Gizmo.OnMouseRelease(PreviousGizmoDirection);
			}
			if (!ImGui::GetIO().WantCaptureMouse && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
			{
				UOverlayManager& Overlay = UOverlayManager::GetInstance();
				FVector MousePos = InputManager.GetMousePosition();

				FScopeCycleCounter PickCounter{ FScopeCycleCounter(TStatId()) };


				Overlay.TotalPickCount++;
				float Distance;
				//UPrimitiveComponent* PrimitiveCollided = InLevel->GetPrimitiveCollided(WorldRay, Distance);
				UPrimitiveComponent* PrimitiveCollided = Renderer.GetCollidedPrimitive(MousePos.X, MousePos.Y);

				Overlay.LastPickTime = PickCounter.Finish();
				Overlay.TotalPickTime += Overlay.LastPickTime;

				if (PrimitiveCollided)
				{
					if (PrimitiveCollided->GetOwner() == SelectedActor)
					{
						UIManager.SetSelectedComponent(PrimitiveCollided);
					}
					else
					{
						UIManager.SetSelectedActor(PrimitiveCollided->GetOwner());
					}
				}
				else
				{
					UIManager.SetSelectedActor(nullptr);
				}
			}
		}
		/** 기즈모가 선택되었을 때. Actor가 선택되지 않으면 기즈모도 선택되지 않으므로 이미 Actor가 선택된 상황. */
		/** SelectedActor를 update하지 않고 마우스 인풋에 따라 hovering or drag */
		else
		{
			PreviousGizmoDirection = Gizmo.GetGizmoDirection();
			/** 드래그 */
			if (InputManager.IsKeyPressed(EKeyInput::MouseLeft))
			{
				Gizmo.OnMouseDragStart(CollisionPoint, SelectedComponent);
			}
			else
			{
				Gizmo.OnMouseHovering();
			}
		}
	}
}

TArray<UPrimitiveComponent*> UEditor::FindCandidatePrimitives(ULevel* InLevel)
{
	TArray<UPrimitiveComponent*> Candidate;

	for (AActor* Actor : InLevel->GetLevelActors())
	{
		for (auto& ActorComponent : Actor->GetOwnedComponents())
		{
			UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(ActorComponent);
			if (Primitive)
			{
				Candidate.push_back(Primitive);
			}
		}
	}

	return Candidate;
}

FVector UEditor::GetGizmoDragLocation(const FRay& WorldRay, const UCamera& ViewCam, const USceneComponent* SelectedComponent)
{
	FMatrix ParentMatrixInverse{ FMatrix::Identity };
	if (SelectedComponent->GetParentComponent())
	{
		ParentMatrixInverse = SelectedComponent->GetParentComponent()->GetWorldTransformMatrixInverse();
	}
	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector AxisWorld = Gizmo.GetGizmoAxis(); // (1,0,0) or (0,1,0) or (0,0,1)


	if (Gizmo.IsWorld())
	{

		// Compute camera forward from view camera rotation (X-forward convention)
		FVector4 CamForward4 = FVector4(1, 0, 0, 1) * FMatrix::RotationMatrixCamera(FVector::GetDegreeToRadian(ViewCam.GetRotation()));
		FVector CamForward = FVector(CamForward4.X, CamForward4.Y, CamForward4.Z);
		CamForward.Normalize();
		FVector PlaneNormal = (CamForward.Cross(AxisWorld)).Cross(AxisWorld);
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
		{
			//이건 월드 좌표계 이동벡터임. 부모 모델 matrix를 곱해서 로컬로 바꿔야함
			//월드 좌표계 기준으로 그냥 이동해버리면 부모 월드 transform을 곱하면서 축이 바뀌고 월드처럼 이동을 안함.
			//부모 월드 transform을 이동벡터에 곱해줌으로써 월드 좌표계로 이동만 한 것처럼 됨.
			//월드축 Transform * 부모 월드 Transform = 부모 로컬 Transform
			FVector MouseDistanceLocal = MouseWorld - Gizmo.GetDragStartMouseLocation();
			MouseDistanceLocal = (FVector4(AxisWorld * MouseDistanceLocal.Dot(AxisWorld), 0.0f) * ParentMatrixInverse).ToVector();
			return Gizmo.GetDragStartActorLocation() + MouseDistanceLocal;
		}
	}
	else
	{
		//본인 월드 transform을 미리 곱해야 원하는 로컬 축으로 이동이 가능함(Plane도 로컬 축으로 계산해야하므로. 곱 안하면 부모 로컬로 이동함)
		//이후에 부모의 월드 transform의 역행렬을 이동벡터에 곱해줌으로써 부모의 transform 행렬이 2번 계산되는 것을 막음.(== 본인 로컬만 적용하고 이후 부모 transform 적용)
		// (본인 월드 Transform * 부모 월드 TransformInverse) * 부모 월드 Transform == 본인 로컬(부모 제외) Transform * 부모 월드 Transform == 본인 World Transform
		//MouseWorld를 AxisWorld로 구한 다음 로컬 축으로만 이동시켜도 됨(주석으로 이 코드도 작성해놓음)

		//FVector AxisLocal = SelectedComponent->GetRelativeRotationQuat().RotateVector(AxisWorld);(본인 로컬만 적용한 버전, 마우스 좌표는 전체 로컬로 구해야함)
		AxisWorld = FMatrix::ToQuat(SelectedComponent->GetWorldTransformMatrix()).RotateVector(AxisWorld);
		AxisWorld.Normalize();


		FVector4 CamForward4 = FVector4(1, 0, 0, 1) * FMatrix::RotationMatrixCamera(FVector::GetDegreeToRadian(ViewCam.GetRotation()));
		FVector CamForward = FVector(CamForward4.X, CamForward4.Y, CamForward4.Z);
		CamForward.Normalize();
		FVector PlaneNormal = (CamForward.Cross(AxisWorld)).Cross(AxisWorld);
		//월드 공간에서 충돌 계산
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
		{
			FVector MouseDistanceLocal = MouseWorld - Gizmo.GetDragStartMouseLocation();
			//return Gizmo.GetDragStartActorLocation() + AxisLocal * MouseDistanceLocal.Dot(AxisLocal);(본인 로컬만 적용한 버전)
			MouseDistanceLocal = (FVector4(AxisWorld * MouseDistanceLocal.Dot(AxisWorld), 0.0f) * ParentMatrixInverse).ToVector();
			return Gizmo.GetDragStartActorLocation() + MouseDistanceLocal;

		}

	}
	return Gizmo.GetGizmoLocation();
}

FQuat UEditor::GetGizmoDragRotationQuat(const FRay& WorldRay, const UCamera& /*ViewCam*/c, const USceneComponent* SelectedComponent)
{
	FMatrix ParentMatrixInverse{ FMatrix::Identity };
	if (SelectedComponent->GetParentComponent())
	{
		ParentMatrixInverse = SelectedComponent->GetParentComponent()->GetWorldTransformMatrixInverse();
	}

	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector AxisWorld = Gizmo.GetGizmoAxis(); // (1,0,0) or (0,1,0) or (0,0,1)

	FVector GizmoAxis = AxisWorld;
	if (!Gizmo.IsWorld())
	{
		//Lotation과 똑같이 로컬의 경우 본인 rotation을 축에 곱해줘야 로컬 회전 가능
		GizmoAxis = FMatrix::ToQuat(SelectedComponent->GetWorldTransformMatrix()).RotateVector(AxisWorld);

		GizmoAxis.Normalize();
	}
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, GizmoAxis, MouseWorld))
	{
		FVector V1 = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		FVector V2 = MouseWorld - PlaneOrigin;
		V1.Normalize();
		V2.Normalize();

		float Angle = acosf(clamp(V1.Dot(V2), -1.0f, 1.0f));
		if (V2.Cross(V1).Dot(GizmoAxis) > 0)
		{
			Angle = -Angle;
		}

		// 드래그 시작 시점의 오리엔테이션
		const FQuat QuatDragStart = Gizmo.GetDragStartActorRotationQuat();
		// 델타 회전 쿼터니언 구성
		// 월드: 월드축 기준 델타, 좌곱
		// 로컬: 로컬축 기준 델타, 우곱
		if (Gizmo.IsWorld())
		{
			//부모 컴포넌트에 의해 회전 축이 바뀔 것이므로 translation과 마찬가지로 미리 역행렬을 곱해줘서 상쇄시킴
			AxisWorld = (FVector4(AxisWorld, 0) * ParentMatrixInverse).ToVector();
			AxisWorld.Normalize();
			const FQuat QuatDelta = FQuat::FromAxisAngle(AxisWorld, Angle);
			return QuatDelta * QuatDragStart;
		}
		else
		{
			//DragStart을 먼저 곱하고(행벡터 우선 행렬의 반대) 월드 축을 기준으로 회전하는 쿼터니언을 뒤에 적용해주면 로컬 축 회전이 됨.
			//그럼 굳이 월드transform을 구할 필요가 없어보이지만 translation과 마찬가지로 회전 축은 미리 원하는 축으로 계산을 해줘야함.
			//Translation과 다르게 곱셈 순서가 로컬, 월드를 결정해서 부모의 역행렬을 곱해 상쇄시켜서 로컬축으로 회전하는 방식은 불가능함.

			const FQuat QuatDeltaLocal = FQuat::FromAxisAngle(AxisWorld, Angle);
			return QuatDragStart * QuatDeltaLocal;
		}
	}
	return Gizmo.GetActorRotationQuat();
}

FVector UEditor::GetGizmoDragScale(const FRay& WorldRay, const UCamera& ViewCam, const USceneComponent* SelectedComponent)
{
	FVector MouseWorld;
	FVector PlaneOrigin{Gizmo.GetGizmoLocation()};
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	// 스케일은 항상 로컬 기준으로 작동: 시작 시점 로컬축을 월드로 고정
	GizmoAxis = FMatrix::ToQuat(SelectedComponent->GetWorldTransformMatrix()).RotateVector(GizmoAxis);

	// Compute camera forward from view camera rotation (X-forward convention)
	FVector4 CamForward4 = FVector4(1, 0, 0, 1) * FMatrix::RotationMatrixCamera(FVector::GetDegreeToRadian(ViewCam.GetRotation()));
	FVector CamForward = FVector(CamForward4.X, CamForward4.Y, CamForward4.Z);
	CamForward.Normalize();
	FVector PlaneNormal = (CamForward.Cross(GizmoAxis)).Cross(GizmoAxis);
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		float DragStartAxisDistance = PlaneOriginToMouseStart.Dot(GizmoAxis);
		float DragAxisDistance = PlaneOriginToMouse.Dot(GizmoAxis);
		float ScaleFactor = 1.0f;
		if (abs(DragStartAxisDistance) > 0.1f)
		{
			ScaleFactor = DragAxisDistance / DragStartAxisDistance;
		}

		FVector DragStartScale = Gizmo.GetDragStartActorScale();
		if (ScaleFactor > MinScale)
		{
			// Uniform이면 모든 축 동일 비율 스케일
			if (UUIManager::GetInstance().GetSelectedComponent()->IsUniformScale())
			{
				return {
					DragStartScale.X * ScaleFactor,
					DragStartScale.Y * ScaleFactor,
					DragStartScale.Z * ScaleFactor
				};
			}

			// 비균일: 선택 축의 성분만 스케일 변경
			FVector NewScale = DragStartScale;
			switch (Gizmo.GetGizmoDirection())
			{
			case EGizmoDirection::Right:
				NewScale.X = max(DragStartScale.X * ScaleFactor, MinScale);
				break;
			case EGizmoDirection::Up:
				NewScale.Y = max(DragStartScale.Y * ScaleFactor, MinScale);
				break;
			case EGizmoDirection::Forward:
				NewScale.Z = max(DragStartScale.Z * ScaleFactor, MinScale);
				break;
			default: break;
			}
			return NewScale;
		}
		return Gizmo.GetActorScale();
	}
	return Gizmo.GetActorScale();
}

// 레벨이 카메라를 가지도록 보장
void UEditor::EnsureLevelHasCamera(ULevel* Level)
{
	if (!Level) return;

	// 포인터 주입
	if (Level->GetCamera() != Camera)
	{
		Level->SetCamera(Camera);
	}
	// 매 프레임 '안전 적용' 시도: 더티면 한 번만 적용되고 플래그가 꺼짐
	Level->TryApplySavedCameraSnapshot();
}
