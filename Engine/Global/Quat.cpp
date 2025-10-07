#include "pch.h"
#include "Global/Quat.h"
#include "Global/Vector.h"
#include "Global/Matrix.h"

const FQuat FQuat::Identity = FQuat(0.0f, 0.0f, 0.0f, 1.0f);
const FQuat FQuat::Zero = FQuat(0.0f, 0.0f, 0.0f, 0.0f);

FQuat FQuat::FromAxisAngle(const FVector& Axis, float Radians)
{
	FVector N = Axis;
	N.Normalize();
	const float Half = 0.5f * Radians;
	const float S = std::sinf(Half);
	const float C = std::cosf(Half);
	return {N.X * S, N.Y * S, N.Z * S, C};
}

static FQuat QuatFromEulerRadians(const FVector& Radians)
{
	// XYZ intrinsic (roll=Z, pitch=X, yaw=Y) 조합과 일관되도록 엔진 표준에 맞춰 적용
	// 여기서는 일반적인 순서 X->Y->Z (row-vector 기준 우측곱)로 합성
	const float CX = std::cosf(Radians.X * 0.5f);
	const float SX = std::sinf(Radians.X * 0.5f);
	const float CY = std::cosf(Radians.Y * 0.5f);
	const float SY = std::sinf(Radians.Y * 0.5f);
	const float CZ = std::cosf(Radians.Z * 0.5f);
	const float SZ = std::sinf(Radians.Z * 0.5f);

	// q = qx * qy * qz (row-vector, local-right multiply와 합치기 위해)
	// Hamilton product expanded
	FQuat QX(SX, 0, 0, CX);
	FQuat QY(0, SY, 0, CY);
	FQuat QZ(0, 0, SZ, CZ);
	return QZ * QY * QX;
}

FQuat FQuat::FromEulerXYZ(const FVector& Degrees)
{
	return QuatFromEulerRadians(FVector::GetDegreeToRadian(Degrees));
}

FVector FQuat::ToEulerXYZ(const FQuat& Q)
{
	// Row-major + row-vector, R = Rx * Ry * Rz 기준의 역변환
	FMatrix M = QuatToRotationMatrix(Q);

	float Y = std::asinf(std::clamp(-M.Data[0][2], -1.0f, 1.0f));

	float X, Z;
	const float CY = std::cosf(Y);
	if (std::fabs(CY) > 1e-6f)
	{
		X = std::atan2f(M.Data[1][2], M.Data[2][2]);
		Z = std::atan2f(M.Data[0][1], M.Data[0][0]);
	}
	else
	{
		// Gimbal 근처: z=0으로 두고 x만 계산
		X = std::atan2f(-M.Data[2][1], M.Data[1][1]);
		Z = 0.0f;
	}

	return FVector{
		FVector::GetRadianToDegree(X),
		FVector::GetRadianToDegree(Y),
		FVector::GetRadianToDegree(Z)
	};
}

void FQuat::Normalize()
{
	const float Len2 = X * X + Y * Y + Z * Z + W * W;
	if (Len2 > 0.0f)
	{
		const float Inv = 1.0f / std::sqrtf(Len2);
		X *= Inv;
		Y *= Inv;
		Z *= Inv;
		W *= Inv;
	}
}

FQuat FQuat::Inverse() const
{
	// 단위 쿼터니언 가정
	return Conjugate();
}

FQuat FQuat::operator*(const FQuat& Rhs) const
{
	// Hamilton product (this ∘ Rhs)
	const float NX = W * Rhs.X + X * Rhs.W + Y * Rhs.Z - Z * Rhs.Y;
	const float NY = W * Rhs.Y - X * Rhs.Z + Y * Rhs.W + Z * Rhs.X;
	const float NZ = W * Rhs.Z + X * Rhs.Y - Y * Rhs.X + Z * Rhs.W;
	const float NW = W * Rhs.W - X * Rhs.X - Y * Rhs.Y - Z * Rhs.Z;
	return {NX, NY, NZ, NW};
}

FVector FQuat::RotateVector(const FVector& V) const
{
	// v' = q * v * q^-1, v treated as pure quaternion (x,y,z,0)
	const FQuat QV(V.X, V.Y, V.Z, 0.0f);
	const FQuat Inv = Inverse();
	const FQuat R = (*this) * QV * Inv;
	return {R.X, R.Y, R.Z};
}

FMatrix QuatToRotationMatrix(const FQuat& Q)
{
	// Row-major + row-vector 규약에서는 일반적으로 알려진 column-vector 행렬의 전치가 필요함.
	FQuat NQ = Q;
	NQ.Normalize();
	const float X = NQ.X, Y = NQ.Y, Z = NQ.Z, W = NQ.W;
	const float XX = X + X, YY = Y + Y, ZZ = Z + Z;
	const float WX = W * XX, wy = W * YY, wz = W * ZZ;

	FMatrix M = FMatrix::Identity;
	// Row-major, row-vector: transpose of column-vector form
	M.Data[0][0] = 1.0f - (Y * YY + Z * ZZ);
	M.Data[0][1] = (X * YY + wz); // was 2xy - 2wz -> transpose => 2xy + 2wz
	M.Data[0][2] = (X * ZZ - wy); // was 2xz + 2wy -> transpose => 2xz - 2wy
	M.Data[0][3] = 0.0f;

	M.Data[1][0] = (X * YY - wz);
	M.Data[1][1] = 1.0f - (X * XX + Z * ZZ);
	M.Data[1][2] = (Y * ZZ + WX);
	M.Data[1][3] = 0.0f;

	M.Data[2][0] = (X * ZZ + wy);
	M.Data[2][1] = (Y * ZZ - WX);
	M.Data[2][2] = 1.0f - (X * XX + Y * YY);
	M.Data[2][3] = 0.0f;

	return M;
}

FMatrix QuatToRotationMatrixInverse(const FQuat& Q)
{
	// 회전행렬은 직교행렬 → 역행렬은 전치 = conjugate로도 생성 가능
	FQuat CQ = Q.Conjugate();
	return QuatToRotationMatrix(CQ);
}
