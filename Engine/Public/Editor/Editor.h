#pragma once
#include "Editor/Camera.h"
#include "Editor/Gizmo.h"
#include "Editor/Grid.h"
#include "Editor/Axis.h"
#include "Core/Object.h"
#include "Editor/ObjectPicker.h"

struct FQuat;

class ULineBatchRenderer;
//viewportManager에서 접근 필요
class FViewportManager;

class UEditor : public UObject
{
	DECLARE_CLASS(UEditor, UObject)

public:
	UEditor();
	~UEditor();
	void Initialize();
	void Update();

	// 카메라의 정보를 저장하기 위해 (카메라를 직접 넘기는 것은 위험)
	const FVector& GetCameraLocation();
	UCamera& GetCamera() { return *Camera; }
	const UCamera& GetCamera() const { return *Camera; }


	/** 배칭 렌더링 버전 */
	// void RenderEditor();
	void RenderEditorBatched(const FVector& CameraLocation);
	void EnsureLevelHasCamera(ULevel* Level);

	/**
	 * @brief 에디터 시스템 초기화 (중복 선언 제거)
	 * 뷰포트 매니저 생성 및 설정 로드 포함
	 */

	 /**
	  * @brief 에디터 시스템 종료
	  * 뷰포트 매니저 설정 저장 및 정리 포함
	  */
	void Shutdown();

	/**
	 * @brief 뷰포트 매니저 접근자
	 * 다른 시스템에서 뷰포트 시스템에 접근할 때 사용
	 */
	FViewportManager* GetViewportManager() const { return ViewportManager; }

	/**
	 * @brief 기즈모 접근자
	 * 각 뷰포트에서 기즈모 렌더링을 위해 사용
	 */
	UGizmo* GetGizmo() { return &Gizmo; }
	const UGizmo* GetGizmo() const { return &Gizmo; }

private:

	void ProcessMouseInput(ULevel* InLevel);
	void ProcessKeyboardInput();
	TArray<UPrimitiveComponent*> FindCandidatePrimitives(ULevel* InLevel);

	FVector GetGizmoDragLocation(const FRay& WorldRay, const UCamera& ViewCam, const USceneComponent* SelectedComponent);
	FVector GetGizmoDragRotation(const FRay& WorldRay, const UCamera& ViewCam);
	FQuat GetGizmoDragRotationQuat(const FRay& WorldRay, const UCamera& c, const USceneComponent* SelectedComponent);
	FVector GetGizmoDragScale(const FRay& WorldRay, const UCamera& ViewCam, const USceneComponent* SelectedComponent);

	UCamera* Camera;
	UObjectPicker ObjectPicker;

	/**
	 * @brief 다중 뷰포트 시스템 매니저
	 * 4분할 뷰포트 시스템의 중앙 제어자
	 */
	FViewportManager* ViewportManager = nullptr;

	const float MinScale = 0.01f;
	float CameraDistance = 30;
	FVector Pos{};
	UGizmo Gizmo;
	UAxis Axis;
	UGrid Grid;

	int32 ActiveViewportIndex = 0;
};
