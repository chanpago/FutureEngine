#include "pch.h"
#include "Editor/Camera.h"
#include "Manager/Input/InputManager.h"
#include "Manager/Time/TimeManager.h"
#include "Manager/Path/PathManager.h"
#include "Render/Renderer/Renderer.h"

#include "Components/StaticMeshComponent.h"

#include <algorithm>

IMPLEMENT_CLASS(UCamera, UObject)

void UCamera::Update()
{
	const UInputManager& Input = UInputManager::GetInstance();

	/*
	 * QE 상하는 카메라가 보는 방향과 관계 없이 월드 기준으로 상하로 움직인다.
	 */
	 // UE 기준(X-forward, Z-up)으로 Forward 기준축을 X로 변경
	Forward = FVector4(1, 0, 0, 1) * FMatrix::RotationMatrixCamera(FVector::GetDegreeToRadian(RelativeRotation));
	Forward.Normalize();
	// UE 기준 Up 축: Z
	Up = FVector(0, 0, 1);
	// Right = Up.Cross(Forward);
	Right = Up.Cross(Forward);
	Right.Normalize();

	Up = Forward.Cross(Right);
	 
	//TODO VIEW
	UpdateViewFrustum(Aspect, FovY, NearZ, FarZ);

	/**
	 * @brief 마우스 우클릭을 하고 있는 동안 카메라 제어가 가능합니다.
	 * 마우스 우클릭하고 있고, 다른 viewport를 조작하지 않고 있고, Gui도 입력받지 않고 있어야 움직임, GUI입력을 무시할 것인지는 선택.
	 */
	if (bInputEnabled && Input.IsKeyDown(EKeyInput::MouseRight) && !ImGui::GetIO().WantCaptureMouse)
	{
		
		/**
		 * @brief W, A, S, D 는 각각 카메라의 상, 하, 좌, 우 이동을 담당합니다.
		 */
		bIsMainDrraging = true;
		FVector Direction = { 0, 0, 0 };

		if (Input.IsKeyDown(EKeyInput::A)) { Direction += -Right; }
		if (Input.IsKeyDown(EKeyInput::D)) { Direction += Right; }
		if (Input.IsKeyDown(EKeyInput::W)) { Direction += Forward; }
		if (Input.IsKeyDown(EKeyInput::S)) { Direction += -Forward; }
		if (Input.IsKeyDown(EKeyInput::Q)) { Direction += -Up; }
		if (Input.IsKeyDown(EKeyInput::E)) { Direction += Up; }
		Direction.Normalize();
		RelativeLocation += Direction * CurrentMoveSpeed * DT;

		// 오른쪽 마우스 버튼 + 마우스 휠로 카메라 이동속도 조절
		float WheelDelta = Input.GetMouseWheelDelta();
		if (WheelDelta != 0.0f)
		{
			// 휠 위로 돌리면 속도 증가, 아래로 돌리면 속도 감소
			AdjustMoveSpeed(WheelDelta * SPEED_ADJUST_STEP);
		}

		/**
		* @brief 마우스 위치 변화량을 감지하여 카메라의 회전을 담당합니다.
		*/
		const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();
		RelativeRotation.Y += MouseDelta.X * CurrentMouseSensitivity;
		RelativeRotation.X += MouseDelta.Y * CurrentMouseSensitivity;

		// Yaw 래핑(값이 무한히 커지지 않도록)
		if (RelativeRotation.Y > 180.0f)
		{
			RelativeRotation.Y -= 360.0f;
		}
		if (RelativeRotation.Y < -180.0f)
		{
			RelativeRotation.Y += 360.0f;
		}

		// Pitch 클램프(짐벌 플립 방지)
		RelativeRotation.X = std::min(RelativeRotation.X, 89.0f);
		RelativeRotation.X = std::max(RelativeRotation.X, -89.0f);
	}
	if (Input.IsKeyReleased(EKeyInput::MouseRight))
	{
		bIsMainDrraging = false;
	}

	if (URenderer::GetInstance().GetDeviceResources())
	{
		float Width = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo().Width;
		float Height = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo().Height;
		SetAspect(Width / Height);
	}

	switch (CameraType)
	{
	case ECameraType::ECT_Perspective:
		UpdateMatrixByPers();
		break;
	case ECameraType::ECT_Orthographic:
		UpdateMatrixByOrth();
		break;
	}

	// TEST CODE
	// URenderer::GetInstance().UpdateConstant(ViewProjConstants);

}

void UCamera::UpdateMatrixByPers()

{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix::RotationMatrixInverseCamera(FVector::GetDegreeToRadian(RelativeRotation));
	// 좌표계 기준 변환(B)을 View에 접합(마지막에 적용): pos = pos * World * (View * B) * Proj
	ViewProjConstants.View = T * (R * FMatrix::BasisLHYToUE());

	/**
	 * @brief Projection 행렬 연산
	 * 원근 투영 행렬 (HLSL에서 row-major로 mul(p, M) 일관성 유지)
	 * f = 1 / tan(fovY/2)
	 */
	const float RadianFovY = FVector::GetDegreeToRadian(FovY);
	const float F = 1.0f / std::tanf(RadianFovY * 0.5f);

	FMatrix P = FMatrix::Identity;
	// | f/aspect   0        0         0 |
	// |    0       f        0         0 |
	// |    0       0   zf/(zf-zn)     1 |
	// |    0       0  -zn*zf/(zf-zn)  0 |
	P.Data[0][0] = F / Aspect;
	P.Data[1][1] = F;
	P.Data[2][2] = FarZ / (FarZ - NearZ);
	P.Data[2][3] = 1.0f;
	P.Data[3][2] = (-NearZ * FarZ) / (FarZ - NearZ);
	P.Data[3][3] = 0.0f;

	ViewProjConstants.Projection = P;

	ViewProj = ViewProjConstants.View * ViewProjConstants.Projection;
}

void UCamera::UpdateMatrixByOrth()
{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix::RotationMatrixInverseCamera(FVector::GetDegreeToRadian(RelativeRotation));
	// 좌표계 기준 변환(B)을 View에 접합(마지막에 적용)
	ViewProjConstants.View = T * (R * FMatrix::BasisLHYToUE());

	/**
	 * @brief Projection 행렬 연산 (월드 단위 기반)
	 */
	OrthoWidth = OrthoWorldWidth;
	const float OrthoHeight = OrthoWidth / Aspect;
	const float Left = -OrthoWidth * 0.5f;
	const float Right1 = OrthoWidth * 0.5f;
	const float Bottom = -OrthoHeight * 0.5f;
	const float Top = OrthoHeight * 0.5f;

	FMatrix P = FMatrix::Identity;
	P.Data[0][0] = 2.0f / (Right1 - Left);
	P.Data[1][1] = 2.0f / (Top - Bottom);
	P.Data[2][2] = 1.0f / (FarZ - NearZ);
	P.Data[3][0] = -(Right1 + Left) / (Right1 - Left);
	P.Data[3][1] = -(Top + Bottom) / (Top - Bottom);
	P.Data[3][2] = -NearZ / (FarZ - NearZ);
	P.Data[3][3] = 1.0f;
	ViewProjConstants.Projection = P;
}

FViewProjConstants UCamera::GetFViewProjConstantsInverse() const
{
	/*
	* @brief View^(-1) = R * T
	*/
	FViewProjConstants Result = {};
	FMatrix R = FMatrix::RotationMatrixCamera(FVector::GetDegreeToRadian(RelativeRotation));
	FMatrix T = FMatrix::TranslationMatrix(RelativeLocation);
	// (View * B)^-1 = B^-1 * View^-1
	Result.View = (FMatrix::BasisUEToLHY() * R) * T;

	if (CameraType == ECameraType::ECT_Orthographic)
	{
		const float OrthoHeight = OrthoWidth / Aspect;
		const float Left = -OrthoWidth * 0.5f;
		const float Right1 = OrthoWidth * 0.5f;
		const float Bottom = -OrthoHeight * 0.5f;
		const float Top = OrthoHeight * 0.5f;

		FMatrix P = FMatrix::Identity;
		// A^{-1} (대각)
		P.Data[0][0] = (Right1 - Left) * 0.5f; // (r-l)/2
		P.Data[1][1] = (Top - Bottom) * 0.5f; // (t-b)/2
		P.Data[2][2] = (FarZ - NearZ); // (zf-zn)
		// -b A^{-1} (마지막 행의 x,y,z)
		P.Data[3][0] = (Right1 + Left) * 0.5f; // (r+l)/2
		P.Data[3][1] = (Top + Bottom) * 0.5f; // (t+b)/2
		P.Data[3][2] = NearZ; // zn
		P.Data[3][3] = 1.0f;
		Result.Projection = P;
	}
	else if ((CameraType == ECameraType::ECT_Perspective))
	{
		const float FovRadian = FVector::GetDegreeToRadian(FovY);
		const float F = 1.0f / std::tanf(FovRadian * 0.5f);
		FMatrix P = FMatrix::Identity;
		// | aspect/F   0      0         0 |
		// |    0      1/F     0         0 |
		// |    0       0      0   -(zf-zn)/(zn*zf) |
		// |    0       0      1        zf/(zn*zf)  |
		P.Data[0][0] = Aspect / F;
		P.Data[1][1] = 1.0f / F;
		P.Data[2][2] = 0.0f;
		P.Data[2][3] = -(FarZ - NearZ) / (NearZ * FarZ);
		P.Data[3][2] = 1.0f;
		P.Data[3][3] = FarZ / (NearZ * FarZ);
		Result.Projection = P;
	}

	return Result;
}


FRay UCamera::ConvertToWorldRay(float NdcX, float NdcY) const
{
	/* *
	 * @brief 반환할 타입의 객체 선언
	 */
	FRay Ray = {};

	FViewProjConstants ViewProjMatrix = GetFViewProjConstantsInverse();
	// ViewProjMatrix.View = ViewProjMatrix.View;

	/* *
	 * @brief NDC 좌표 정보를 행렬로 변환합니다.
	 */
	const FVector4 NdcNear(NdcX, NdcY, 0.0f, 1.0f);
	const FVector4 NdcFar(NdcX, NdcY, 1.0f, 1.0f);

	/* *
	 * @brief Projection 행렬을 View 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 ViewNear = MultiplyPointWithMatrix(NdcNear, ViewProjMatrix.Projection);
	const FVector4 ViewFar = MultiplyPointWithMatrix(NdcFar, ViewProjMatrix.Projection);

	/* *
	 * @brief View 행렬을 World 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 WorldNear = MultiplyPointWithMatrix(ViewNear, ViewProjMatrix.View);
	const FVector4 WorldFar = MultiplyPointWithMatrix(ViewFar, ViewProjMatrix.View);

	/* *
	 * @brief 카메라의 월드 좌표를 추출합니다.
	 * Row-major 기준, 마지막 행 벡터는 위치 정보를 가지고 있음
	 */
	const FVector4 CameraPosition(
		ViewProjMatrix.View.Data[3][0],
		ViewProjMatrix.View.Data[3][1],
		ViewProjMatrix.View.Data[3][2],
		ViewProjMatrix.View.Data[3][3]);

	if (CameraType == ECameraType::ECT_Perspective)
	{
		FVector4 DirectionVector = WorldFar - WorldNear;
		DirectionVector.Normalize();

		Ray.Origin = WorldNear;
		Ray.Direction = DirectionVector;
	}
	else if (CameraType == ECameraType::ECT_Orthographic)
	{
		FVector4 DirectionVector = WorldFar - WorldNear;
		DirectionVector.Normalize();

		Ray.Origin = WorldNear;
		Ray.Direction = DirectionVector;
	}

	// 기준변환을 View에 흡수했으므로 별도 축 순열 변환 불필요
	Ray.Direction.Normalize();

	return Ray;
}

FVector UCamera::CalculatePlaneNormal(const FVector4& Axis) const
{
	return Forward.Cross(FVector(Axis.X, Axis.Y, Axis.Z));
}

FVector UCamera::CalculatePlaneNormal(const FVector& Axis) const
{
	return Forward.Cross(FVector(Axis.X, Axis.Y, Axis.Z));
}

void UCamera::SaveCameraSettings() const
{
	const path ConfigFilePath = UPathManager::GetInstance().GetConfigPath() / "editor.ini";

	WritePrivateProfileStringA(
		"Camera",
		"MoveSpeed",
		std::to_string(CurrentMoveSpeed).c_str(),
		ConfigFilePath.string().c_str()
	);

	WritePrivateProfileStringA(
		"Camera",
		"MouseSensitivity",
		std::to_string(CurrentMouseSensitivity).c_str(),
		ConfigFilePath.string().c_str()
	);
}

void UCamera::LoadCameraSettings()
{
	const path ConfigFilePath = UPathManager::GetInstance().GetConfigPath() / "editor.ini";

	// Check if config file exists
	if (!std::filesystem::exists(ConfigFilePath))
	{
		// Create default config if it doesn't exist
		SaveCameraSettings();
		return;
	}

	char Buffer[32];

	// Load Move Speed
	GetPrivateProfileStringA(
		"Camera",
		"MoveSpeed",
		std::to_string(DEFAULT_CAMERA_SPEED).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);

	float LoadedSpeed = std::stof(Buffer);
	// 로드한 값을 직접 설정 (SaveCameraSettings 호출하지 않음)
	CurrentMoveSpeed = max(LoadedSpeed, MIN_CAMERA_SPEED);
	CurrentMoveSpeed = min(CurrentMoveSpeed, MAX_CAMERA_SPEED);

	// Load Mouse Sensitivity
	GetPrivateProfileStringA(
		"Camera",
		"MouseSensitivity",
		std::to_string(DEFAULT_MOUSE_SENSITIVITY).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);

	float LoadedSensitivity = std::stof(Buffer);
	// 로드한 값을 직접 설정 (SaveCameraSettings 호출하지 않음)
	CurrentMouseSensitivity = max(LoadedSensitivity, MIN_MOUSE_SENSITIVITY);
	CurrentMouseSensitivity = min(CurrentMouseSensitivity, MAX_MOUSE_SENSITIVITY);
}

void UCamera::SaveMultiViewCameraSettings(const char* ViewportType) const
{
	const path ConfigFilePath = UPathManager::GetInstance().GetRootPath() / "Editor.ini";

	// 각 뷰포트별로 섹션을 구분 (예: "Camera_Perspective", "Camera_Top" 등)
	std::string SectionName = std::string("Camera_") + ViewportType;

	// 카메라 위치 저장
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"LocationX",
		std::to_string(RelativeLocation.X).c_str(),
		ConfigFilePath.string().c_str()
	);
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"LocationY",
		std::to_string(RelativeLocation.Y).c_str(),
		ConfigFilePath.string().c_str()
	);
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"LocationZ",
		std::to_string(RelativeLocation.Z).c_str(),
		ConfigFilePath.string().c_str()
	);

	// 카메라 회전 저장
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"RotationX",
		std::to_string(RelativeRotation.X).c_str(),
		ConfigFilePath.string().c_str()
	);
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"RotationY",
		std::to_string(RelativeRotation.Y).c_str(),
		ConfigFilePath.string().c_str()
	);
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"RotationZ",
		std::to_string(RelativeRotation.Z).c_str(),
		ConfigFilePath.string().c_str()
	);

	// FOV 저장 (Perspective 뷰포트에만 적용)
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"FovY",
		std::to_string(FovY).c_str(),
		ConfigFilePath.string().c_str()
	);

	// 카메라 타입 저장
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"CameraType",
		(CameraType == ECameraType::ECT_Perspective) ? "Perspective" : "Orthographic",
		ConfigFilePath.string().c_str()
	);

	// Orthographic 전용: 월드 너비 저장
	WritePrivateProfileStringA(
		SectionName.c_str(),
		"OrthoWorldWidth",
		std::to_string(OrthoWorldWidth).c_str(),
		ConfigFilePath.string().c_str()
	);
}

void UCamera::LoadMultiViewCameraSettings(const char* ViewportType)
{
	const path ConfigFilePath = UPathManager::GetInstance().GetRootPath() / "Editor.ini";

	// 파일이 존재하지 않으면 기본값 유지
	if (!std::filesystem::exists(ConfigFilePath))
	{
		return;
	}

	std::string SectionName = std::string("Camera_") + ViewportType;
	char Buffer[64];

	// 카메라 위치 로드
	GetPrivateProfileStringA(
		SectionName.c_str(),
		"LocationX",
		std::to_string(RelativeLocation.X).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	RelativeLocation.X = std::stof(Buffer);

	GetPrivateProfileStringA(
		SectionName.c_str(),
		"LocationY",
		std::to_string(RelativeLocation.Y).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	RelativeLocation.Y = std::stof(Buffer);

	GetPrivateProfileStringA(
		SectionName.c_str(),
		"LocationZ",
		std::to_string(RelativeLocation.Z).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	RelativeLocation.Z = std::stof(Buffer);

	// 카메라 회전 로드
	GetPrivateProfileStringA(
		SectionName.c_str(),
		"RotationX",
		std::to_string(RelativeRotation.X).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	RelativeRotation.X = std::stof(Buffer);

	GetPrivateProfileStringA(
		SectionName.c_str(),
		"RotationY",
		std::to_string(RelativeRotation.Y).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	RelativeRotation.Y = std::stof(Buffer);

	GetPrivateProfileStringA(
		SectionName.c_str(),
		"RotationZ",
		std::to_string(RelativeRotation.Z).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	RelativeRotation.Z = std::stof(Buffer);

	// FOV 로드
	GetPrivateProfileStringA(
		SectionName.c_str(),
		"FovY",
		std::to_string(FovY).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	FovY = std::stof(Buffer);

	// 카메라 타입 로드
	GetPrivateProfileStringA(
		SectionName.c_str(),
		"CameraType",
		"Perspective",
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	std::string CamType = Buffer;
	CameraType = (CamType == "Perspective") ? ECameraType::ECT_Perspective : ECameraType::ECT_Orthographic;

	// Orthographic 전용: 월드 너비 로드
	GetPrivateProfileStringA(
		SectionName.c_str(),
		"OrthoWorldWidth",
		std::to_string(OrthoWorldWidth).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);
	OrthoWorldWidth = std::stof(Buffer);
}

void UCamera::SaveMultiViewCameraState(const UCamera* Camera, const char* ViewportType)
{
	if (!Camera) return;

	const path ConfigFilePath = UPathManager::GetInstance().GetRootPath() / "Editor.ini";
	std::string SectionName = std::string("Camera_") + ViewportType;

	// 정적 함수에서는 카메라 객체의 상태를 직접 저장
	WritePrivateProfileStringA(SectionName.c_str(), "LocationX", std::to_string(Camera->GetLocation().X).c_str(), ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "LocationY", std::to_string(Camera->GetLocation().Y).c_str(), ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "LocationZ", std::to_string(Camera->GetLocation().Z).c_str(), ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "RotationX", std::to_string(Camera->GetRotation().X).c_str(), ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "RotationY", std::to_string(Camera->GetRotation().Y).c_str(), ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "RotationZ", std::to_string(Camera->GetRotation().Z).c_str(), ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "FovY", std::to_string(Camera->GetFovY()).c_str(), ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "CameraType", (Camera->GetCameraType() == ECameraType::ECT_Perspective) ? "Perspective" : "Orthographic", ConfigFilePath.string().c_str());
	WritePrivateProfileStringA(SectionName.c_str(), "OrthoWorldWidth", std::to_string(Camera->GetOrthoWorldWidth()).c_str(), ConfigFilePath.string().c_str());
}

void UCamera::LoadMultiViewCameraState(UCamera* Camera, const char* ViewportType)
{
	if (!Camera) return;

	const path ConfigFilePath = UPathManager::GetInstance().GetRootPath() / "Editor.ini";

	if (!std::filesystem::exists(ConfigFilePath))
	{
		return;
	}

	std::string SectionName = std::string("Camera_") + ViewportType;
	char Buffer[64];

	// 위치 로드
	FVector LoadedLocation;
	GetPrivateProfileStringA(SectionName.c_str(), "LocationX", std::to_string(Camera->GetLocation().X).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	LoadedLocation.X = std::stof(Buffer);
	GetPrivateProfileStringA(SectionName.c_str(), "LocationY", std::to_string(Camera->GetLocation().Y).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	LoadedLocation.Y = std::stof(Buffer);
	GetPrivateProfileStringA(SectionName.c_str(), "LocationZ", std::to_string(Camera->GetLocation().Z).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	LoadedLocation.Z = std::stof(Buffer);
	Camera->SetLocation(LoadedLocation);

	// 회전 로드
	FVector LoadedRotation;
	GetPrivateProfileStringA(SectionName.c_str(), "RotationX", std::to_string(Camera->GetRotation().X).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	LoadedRotation.X = std::stof(Buffer);
	GetPrivateProfileStringA(SectionName.c_str(), "RotationY", std::to_string(Camera->GetRotation().Y).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	LoadedRotation.Y = std::stof(Buffer);
	GetPrivateProfileStringA(SectionName.c_str(), "RotationZ", std::to_string(Camera->GetRotation().Z).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	LoadedRotation.Z = std::stof(Buffer);
	Camera->SetRotation(LoadedRotation);

	// FOV 로드
	GetPrivateProfileStringA(SectionName.c_str(), "FovY", std::to_string(Camera->GetFovY()).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	Camera->SetFovY(std::stof(Buffer));

	// 카메라 타입 로드
	GetPrivateProfileStringA(SectionName.c_str(), "CameraType", "Perspective", Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	std::string CamType = Buffer;
	Camera->SetCameraType((CamType == "Perspective") ? ECameraType::ECT_Perspective : ECameraType::ECT_Orthographic);

	// Orthographic 전용: 월드 너비 로드
	GetPrivateProfileStringA(SectionName.c_str(), "OrthoWorldWidth", std::to_string(Camera->GetOrthoWorldWidth()).c_str(), Buffer, sizeof(Buffer), ConfigFilePath.string().c_str());
	Camera->SetOrthoWorldWidth(std::stof(Buffer));
}

float UCamera::SignedDistance_Point_Plane(const Plane& pl, const FVector& p)
{ 
	// Plane: dot(N, X) = D  →  signed distance = dot(N, X) - D
	float rtn = pl.Normal.Dot(p) - pl.Distance;

	return rtn;
}

FVector UCamera::AABB_NegativeVertex(const FAABB& box, const FVector& n)
{
 
	return FVector{
		(n.X >= 0.0f) ? box.Min.X : box.Max.X,
		(n.Y >= 0.0f) ? box.Min.Y : box.Max.Y,
		(n.Z >= 0.0f) ? box.Min.Z : box.Max.Z
	};
	 
}

void UCamera::UpdateViewFrustum(float aspect, float fovYDeg, float zNear, float zFar)
{ 
	const float fovY = FVector::GetDegreeToRadian(fovYDeg);
	const float HalfVSide = tan(fovY * 0.5f) * zFar;
	const float HalfHSide = HalfVSide * aspect;

	const FVector CamToFar = Forward * zFar;
	const FVector NearCenter = RelativeLocation + Forward * zNear;
	const FVector FarCenter = RelativeLocation + Forward * zFar;

	FVector RightNormal = (CamToFar + Right * HalfHSide).Cross(Up);
	RightNormal.Normalize();

	FVector LeftNormal = (Up).Cross(CamToFar - Right * HalfHSide);
	LeftNormal.Normalize();

	FVector TopNormal = (Right).Cross(CamToFar + Up * HalfVSide);
	TopNormal.Normalize();

	FVector BottomFace = (CamToFar - Up * HalfVSide).Cross(Right);
	BottomFace.Normalize();

	ViewFrustum.NearFace = { NearCenter, Forward};
	ViewFrustum.FarFace = { FarCenter, -Forward};
	ViewFrustum.RightFace = { RelativeLocation, RightNormal};
	ViewFrustum.LeftFace =  { RelativeLocation , LeftNormal};
	ViewFrustum.TopFace = { RelativeLocation,  TopNormal};
	ViewFrustum.BottomFace = { RelativeLocation, BottomFace};
	 
}

bool UCamera::IsOnFrustum(UPrimitiveComponent* PrimitiveComponent)
{
	FVector Location = PrimitiveComponent->GetRelativeLocation();
	FAABB AABB = PrimitiveComponent->GetWorldBounds();

	return IsOnorForwardPlane(AABB); 
}

bool UCamera::IsOnFrustum(const FAABB& AABB)
{
	return IsOnorForwardPlane(AABB);
}

bool UCamera::IsOnorForwardPlane(const FAABB& AABB)
{
	return IsOnOrForwardPlane(ViewFrustum.NearFace, AABB) &&
		IsOnOrForwardPlane(ViewFrustum.FarFace, AABB) &&
		IsOnOrForwardPlane(ViewFrustum.BottomFace, AABB) &&
		IsOnOrForwardPlane(ViewFrustum.TopFace, AABB) &&
		IsOnOrForwardPlane(ViewFrustum.RightFace, AABB) &&
		IsOnOrForwardPlane(ViewFrustum.LeftFace, AABB);
}

bool UCamera::IsOnOrForwardPlane(const Plane& plane, const FAABB& box) 
{
	const float r = box.GetExtent().X* std::abs(plane.Normal.X) +
		box.GetExtent().Y * std::abs(plane.Normal.Y) +
		box.GetExtent().Z * std::abs(plane.Normal.Z);

	return -r < SignedDistance_Point_Plane(plane, box.GetCenter());
}
