#include "pch.h"
#include "Editor/Camera.h"
#include "Render/UI/Widget/CameraControlWidget.h"

IMPLEMENT_CLASS(UCameraControlWidget, UWidget)

// Camera Mode
static const char* CameraMode[] = {
	"Perspective",
	"Orthographic"
};

UCameraControlWidget::UCameraControlWidget()
{
}

UCameraControlWidget::~UCameraControlWidget() = default;

void UCameraControlWidget::Initialize()
{
}

void UCameraControlWidget::Update()
{
}


void UCameraControlWidget::RenderWidget()
{
	/*
	* 새 씬 생성시, 카메라 초기 기본 값으로 두기 위하여
	* 이전의 캐시 값을 활용하는 것을 버리고
	* 무상태(stateless)로 만듬
	*/
	if (!Camera) {
		ImGui::TextUnformatted("Camera not set.");
		ImGui::Separator();
		ImGui::TextUnformatted("Call SetCamera(camera*) after creating this window.");
		return;
	}

	ImGui::TextUnformatted("Camera Transform");
	ImGui::Spacing();

	// 이동속도
	{
		float speed = Camera->GetMoveSpeed();
		if (ImGui::SliderFloat("이동속도", &speed, 0.5f, 50.0f, "%.1f"))
			Camera->SetMoveSpeed(speed);
	}

	// 마우스 감도
	{
		float sens = Camera->GetMouseSensitivity();
		if (ImGui::SliderFloat("마우스 감도", &sens, 0.001f, 0.5f, "%.3f"))
			Camera->SetMouseSensitivity(sens);
	}

	ImGui::Spacing();

	// 모드 (매 프레임 읽기)
	int modeIndex = (Camera->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	if (ImGui::Combo("Mode", &modeIndex, CameraMode, IM_ARRAYSIZE(CameraMode))) {
		Camera->SetCameraType(modeIndex == 0 ? ECameraType::ECT_Perspective : ECameraType::ECT_Orthographic);
		(modeIndex == 0) ? Camera->UpdateMatrixByPers() : Camera->UpdateMatrixByOrth();
	}

	// 위치
	{
		FVector loc = Camera->GetLocation();
		if (ImGui::DragFloat3("Camera Location", &loc.X, 0.05f)) {
			Camera->SetLocation(loc);
			(modeIndex == 0) ? Camera->UpdateMatrixByPers() : Camera->UpdateMatrixByOrth();
		}
	}

	// 회전 (deg 가정)
	{
		FVector rot = Camera->GetRotation();
		bool changed = ImGui::DragFloat3("Camera Rotation", &rot.X, 0.1f);
		rot.X = std::max(-89.0f, std::min(89.0f, rot.X));
		rot.Y = std::max(-180.0f, std::min(180.0f, rot.Y));
		if (changed) {
			Camera->SetRotation(rot);
			(modeIndex == 0) ? Camera->UpdateMatrixByPers() : Camera->UpdateMatrixByOrth();
		}
	}

	ImGui::TextUnformatted("Camera Optics");
	ImGui::Spacing();

	// FOV / Near / Far (매 프레임 읽기)
	{
		float fov = Camera->GetFovY();
		float nearZ = Camera->GetNearZ();
		float farZ = Camera->GetFarZ();

		bool changed = false;
		changed |= ImGui::DragFloat("FOV", &fov, 0.1f, 1.0f, 170.0f, "%.1f");
		changed |= ImGui::DragFloat("Z Near", &nearZ, 0.01f, 0.0001f, 1e6f, "%.4f");
		changed |= ImGui::DragFloat("Z Far", &farZ, 0.1f, 0.001f, 1e7f, "%.3f");

		// 보정
		if (changed) {
			nearZ = std::max(0.0001f, nearZ);
			farZ = std::max(nearZ + 0.0001f, farZ);
			fov = std::clamp(fov, 1.0f, 170.0f);

			Camera->SetFovY(fov);
			Camera->SetNearZ(nearZ);
			Camera->SetFarZ(farZ);

			(modeIndex == 0) ? Camera->UpdateMatrixByPers() : Camera->UpdateMatrixByOrth();
		}
	}

	if (ImGui::Button("Reset Optics")) {
		// 원하는 초기값
		const float fov = 80.0f, nearZ = 0.1f, farZ = 1000.0f;
		Camera->SetFovY(fov);
		Camera->SetNearZ(nearZ);
		Camera->SetFarZ(farZ);
		(modeIndex == 0) ? Camera->UpdateMatrixByPers() : Camera->UpdateMatrixByOrth();
	}

	ImGui::Separator();
}



void UCameraControlWidget::SyncFromCamera()
{
	if (!Camera) { return; }

	CameraModeIndex = (Camera->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	UiFovY = Camera->GetFovY();
	UiNearZ = Camera->GetNearZ();
	UiFarZ = Camera->GetFarZ();
}

void UCameraControlWidget::PushToCamera()
{
	if (!Camera) { return; }

	/*
	 * @brief 카메라 모드 설정
	 */
	Camera->SetCameraType(CameraModeIndex == 0
		                      ? ECameraType::ECT_Perspective
		                      : ECameraType::ECT_Orthographic);

	/*
	 * @brief 카메라 파라미터 설정
	 */
	UiNearZ = std::max(0.0001f, UiNearZ);
	UiFarZ = std::max(UiNearZ + 0.0001f, UiFarZ);
	UiFovY = std::min(170.0f, std::max(1.0f, UiFovY));

	Camera->SetNearZ(UiNearZ);
	Camera->SetFarZ(UiFarZ);
	Camera->SetFovY(UiFovY);

	/*
	 * @brief 카메라 업데이트
	 */
	if (CameraModeIndex == 0)
	{
		Camera->UpdateMatrixByPers();
	}
	else
	{
		Camera->UpdateMatrixByOrth();
	}
}
