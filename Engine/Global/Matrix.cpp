#include "pch.h"


// 상단에 함께 두면 편함
static inline void StoreRow(float* r4, __m128 v) { _mm_storeu_ps(r4, v); }

/**
* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix()
	: Data{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
{
}


/**
* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix(
	float M00, float M01, float M02, float M03,
	float M10, float M11, float M12, float M13,
	float M20, float M21, float M22, float M23,
	float M30, float M31, float M32, float M33)
	: Data{
		{M00, M01, M02, M03},
		{M10, M11, M12, M13},
		{M20, M21, M22, M23},
		{M30, M31, M32, M33}
	}
{
}

/**
 * @brief 행렬의 전치행렬을 반환하는 함수
 */
FMatrix FMatrix::Transpose(const FMatrix& InOtherMatrix)
{
#if defined(__AVX2__)
	__m128 r0 = LoadRow(InOtherMatrix.Data[0]); // [a00 a01 a02 a03]
	__m128 r1 = LoadRow(InOtherMatrix.Data[1]); // [a10 a11 a12 a13]
	__m128 r2 = LoadRow(InOtherMatrix.Data[2]); // [a20 a21 a22 a23]
	__m128 r3 = LoadRow(InOtherMatrix.Data[3]); // [a30 a31 a32 a33]

	_MM_TRANSPOSE4_PS(r0, r1, r2, r3); // r0=[a00 a10 a20 a30] ...

	FMatrix B = FMatrix::Zero;
	StoreRow(B.Data[0], r0);
	StoreRow(B.Data[1], r1);
	StoreRow(B.Data[2], r2);
	StoreRow(B.Data[3], r3);
	return B;
#else
	return {
		InOtherMatrix.Data[0][0], InOtherMatrix.Data[1][0], InOtherMatrix.Data[2][0], InOtherMatrix.Data[3][0],
		InOtherMatrix.Data[0][1], InOtherMatrix.Data[1][1], InOtherMatrix.Data[2][1], InOtherMatrix.Data[3][1],
		InOtherMatrix.Data[0][2], InOtherMatrix.Data[1][2], InOtherMatrix.Data[2][2], InOtherMatrix.Data[3][2],
		InOtherMatrix.Data[0][3], InOtherMatrix.Data[1][3], InOtherMatrix.Data[2][3], InOtherMatrix.Data[3][3]
	};
#endif
}


/**
* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
*/
FMatrix FMatrix::operator*(const FMatrix& InOtherMatrix) const
{
#if defined(__AVX2__)
	// B의 열 4개 미리 준비
	__m128 bc0 = LoadCol(InOtherMatrix.Data, 0);
	__m128 bc1 = LoadCol(InOtherMatrix.Data, 1);
	__m128 bc2 = LoadCol(InOtherMatrix.Data, 2);
	__m128 bc3 = LoadCol(InOtherMatrix.Data, 3);

	FMatrix Out = FMatrix::Zero;
	for (int i = 0; i < 4; ++i)
	{
		__m128 ar = LoadRow(this->Data[i]);
		DotRowByCols_4x(ar, bc0, bc1, bc2, bc3, Out.Data[i]);
	}
	return Out;
#else
	FMatrix Result = FMatrix::Zero;
	for (int32 i = 0; i < 4; ++i)
		for (int32 j = 0; j < 4; ++j)
			for (int32 k = 0; k < 4; ++k)
				Result.Data[i][j] += Data[i][k] * InOtherMatrix.Data[k][j];
	return Result;
#endif
}

void FMatrix::operator*=(const FMatrix& InOtherMatrix)
{
	*this = (*this) * InOtherMatrix;
}

/**
* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::TranslationMatrix(const FVector& InOtherVector)
{
#if defined(__AVX2__)
	FMatrix M = FMatrix::Identity;
	// 마지막 행: [tx, ty, tz, 1]
	__m128 r3 = _mm_set_ps(1.0f, InOtherVector.Z, InOtherVector.Y, InOtherVector.X);
	StoreRow(M.Data[3], r3);
	return M;
#else
	FMatrix M = FMatrix::Identity;
	M.Data[3][0] = InOtherVector.X; M.Data[3][1] = InOtherVector.Y; M.Data[3][2] = InOtherVector.Z; M.Data[3][3] = 1;
	return M;
#endif
}

FMatrix FMatrix::TranslationMatrixInverse(const FVector& InOtherVector)
{
#if defined(__AVX2__)
	FMatrix M = FMatrix::Identity;
	__m128 r3 = _mm_set_ps(1.0f, -InOtherVector.Z, -InOtherVector.Y, -InOtherVector.X);
	StoreRow(M.Data[3], r3);
	return M;
#else
	FMatrix M = FMatrix::Identity;
	M.Data[3][0] = -InOtherVector.X; M.Data[3][1] = -InOtherVector.Y; M.Data[3][2] = -InOtherVector.Z; M.Data[3][3] = 1;
	return M;
#endif
}

/**
* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::ScaleMatrix(const FVector& InOtherVector)
{
#if defined(__AVX2__)
	FMatrix M = FMatrix::Zero;
	StoreRow(M.Data[0], _mm_set_ps(0.0f, 0.0f, 0.0f, InOtherVector.X));
	StoreRow(M.Data[1], _mm_set_ps(0.0f, 0.0f, InOtherVector.Y, 0.0f));
	StoreRow(M.Data[2], _mm_set_ps(0.0f, InOtherVector.Z, 0.0f, 0.0f));
	StoreRow(M.Data[3], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
	return M;
#else
	FMatrix M = FMatrix::Identity;
	M.Data[0][0] = InOtherVector.X; M.Data[1][1] = InOtherVector.Y; M.Data[2][2] = InOtherVector.Z;
	return M;
#endif
}

FMatrix FMatrix::ScaleMatrixInverse(const FVector& InOtherVector)
{
#if defined(__AVX2__)
	const float ix = 1.0f / InOtherVector.X, iy = 1.0f / InOtherVector.Y, iz = 1.0f / InOtherVector.Z;
	FMatrix M = FMatrix::Zero;
	StoreRow(M.Data[0], _mm_set_ps(0.0f, 0.0f, 0.0f, ix));
	StoreRow(M.Data[1], _mm_set_ps(0.0f, 0.0f, iy, 0.0f));
	StoreRow(M.Data[2], _mm_set_ps(0.0f, iz, 0.0f, 0.0f));
	StoreRow(M.Data[3], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
	return M;
#else
	FMatrix M = FMatrix::Identity;
	M.Data[0][0] = 1 / InOtherVector.X; M.Data[1][1] = 1 / InOtherVector.Y; M.Data[2][2] = 1 / InOtherVector.Z;
	return M;
#endif
}

/**
* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::RotationMatrix(const FVector& InOtherVector)
{
    return RotationX(InOtherVector.X) * RotationY(InOtherVector.Y) * RotationZ(InOtherVector.Z);
}

FMatrix FMatrix::RotationMatrixInverse(const FVector& InOtherVector)
{
	return RotationZ(-InOtherVector.Z) * RotationY(-InOtherVector.Y) * RotationX(-InOtherVector.X);
}

/**
* @brief Camera용 Rotation의 정보를 행렬로 변환하여 제공하는 함수
*		 카메라의 경우 YXZ 순서로 회전해야 일반적인 카메라 회전과 동일
*/

FMatrix FMatrix::RotationMatrixCamera(const FVector& InOtherVector)
{
	// UE 기준: Pitch(X) around Y-axis, Yaw(Y) around Z-axis, Roll(Z) around X-axis
	// 적용 순서(행벡터): Yaw -> Pitch -> Roll
	return RotationX(InOtherVector.Z) * RotationY(InOtherVector.X) * RotationZ(InOtherVector.Y);
}

FMatrix FMatrix::RotationMatrixInverseCamera(const FVector& InOtherVector)
{
	// (Yaw*Pitch*Roll)^-1 = Roll^-1 * Pitch^-1 * Yaw^-1
	return RotationZ(-InOtherVector.Y) * RotationY(-InOtherVector.X) * RotationX(-InOtherVector.Z);
}

/**
* @brief X의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationX(float Radian)
{
#if defined(__AVX2__)
	float C = std::cosf(Radian), S = std::sinf(Radian);
	FMatrix M = FMatrix::Identity;
	// row1: [0,C,S,0], row2: [0,-S,C,0]
	StoreRow(M.Data[1], _mm_set_ps(0.0f, S, C, 0.0f));
	StoreRow(M.Data[2], _mm_set_ps(0.0f, C, -S, 0.0f));
	return M;
#else
	FMatrix M = FMatrix::Identity;
	const float C = std::cosf(Radian), S = std::sinf(Radian);
	M.Data[1][1] = C; M.Data[1][2] = S; M.Data[2][1] = -S; M.Data[2][2] = C; return M;
#endif
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationY(float Radian)
{
#if defined(__AVX2__)
	float C = std::cosf(Radian), S = std::sinf(Radian);
	FMatrix M = FMatrix::Identity;
	// row0: [C,0,-S,0], row2: [S,0,C,0]
	StoreRow(M.Data[0], _mm_set_ps(0.0f, -S, 0.0f, C));
	StoreRow(M.Data[2], _mm_set_ps(0.0f, C, 0.0f, S));
	return M;
#else
	FMatrix M = FMatrix::Identity;
	const float C = std::cosf(Radian), S = std::sinf(Radian);
	M.Data[0][0] = C; M.Data[0][2] = -S; M.Data[2][0] = S; M.Data[2][2] = C; return M;
#endif
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationZ(float Radian)
{
#if defined(__AVX2__)
	float C = std::cosf(Radian), S = std::sinf(Radian);
	FMatrix M = FMatrix::Identity;
	// row0: [C,S,0,0], row1: [-S,C,0,0]
	StoreRow(M.Data[0], _mm_set_ps(0.0f, 0.0f, S, C));
	StoreRow(M.Data[1], _mm_set_ps(0.0f, 0.0f, C, -S));
	return M;
#else
	FMatrix M = FMatrix::Identity;
	const float C = std::cosf(Radian), S = std::sinf(Radian);
	M.Data[0][0] = C; M.Data[0][1] = S; M.Data[1][0] = -S; M.Data[1][1] = C; return M;
#endif
}

// Quaternion 기반 회전행렬 (row-major)
FMatrix FMatrix::RotationMatrix(const FQuat& Q)
{
    return QuatToRotationMatrix(Q);
}

FMatrix FMatrix::RotationMatrixInverse(const FQuat& Q)
{
    return QuatToRotationMatrixInverse(Q);
}

FQuat FMatrix::ToQuat(const FMatrix& Mat)
{
	FQuat q;
	// 행렬의 3x3 회전 부분만 사용합니다.
	FVector Row0{ Mat.Data[0][0], Mat.Data[0][1], Mat.Data[0][2] };
	FVector Row1{ Mat.Data[1][0], Mat.Data[1][1], Mat.Data[1][2] };
	FVector Row2{ Mat.Data[2][0], Mat.Data[2][1], Mat.Data[2][2] };
	Row0.Normalize();
	Row1.Normalize();
	Row2.Normalize();
	const float m00 = Row0.X, m01 = Row1.X, m02 = Row2.X;
	const float m10 = Row0.Y, m11 = Row1.Y, m12 = Row2.Y;
	const float m20 = Row0.Z, m21 = Row1.Z, m22 = Row2.Z;

	// 1. 행렬의 대각합(Trace)을 계산합니다.
	const float trace = m00 + m11 + m22;

	if (trace > 0.0f)
	{
		// 경우 1: 트레이스가 0보다 큼 (가장 안정적인 경로)
		float s = sqrt(trace + 1.0f) * 2.0f;
		q.W = 0.25f * s;
		q.X = (m21 - m12) / s;
		q.Y = (m02 - m20) / s;
		q.Z = (m10 - m01) / s;
	}
	else if ((m00 > m11) && (m00 > m22))
	{
		// 경우 2: m00이 가장 큰 대각 원소
		float s = sqrt(1.0f + m00 - m11 - m22) * 2.0f;
		q.W = (m21 - m12) / s;
		q.X = 0.25f * s;
		q.Y = (m01 + m10) / s;
		q.Z = (m02 + m20) / s;
	}
	else if (m11 > m22)
	{
		// 경우 3: m11이 가장 큰 대각 원소
		float s = sqrt(1.0f + m11 - m00 - m22) * 2.0f;
		q.W = (m02 - m20) / s;
		q.X = (m01 + m10) / s;
		q.Y = 0.25f * s;
		q.Z = (m12 + m21) / s;
	}
	else
	{
		// 경우 4: m22가 가장 큰 대각 원소
		float s = sqrt(1.0f + m22 - m00 - m11) * 2.0f;
		q.W = (m10 - m01) / s;
		q.X = (m02 + m20) / s;
		q.Y = (m12 + m21) / s;
		q.Z = 0.25f * s;
	}

	// 부동 소수점 오차를 제거하기 위해 정규화합니다.
	q.Normalize();

	return q;
}

FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
    FMatrix T = TranslationMatrix(Location);
    FMatrix R = RotationMatrix(Rotation);
    FMatrix S = ScaleMatrix(Scale);

    return S * R * T;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrixInverse(Location);
	FMatrix R = RotationMatrixInverse(Rotation);
	FMatrix S = ScaleMatrixInverse(Scale);

    return T * R * S;
}

FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FQuat& Rotation, const FVector& Scale)
{
    FMatrix T = TranslationMatrix(Location);
    FMatrix R = RotationMatrix(Rotation);
    FMatrix S = ScaleMatrix(Scale);
    return S * R * T;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FQuat& Rotation, const FVector& Scale)
{
    FMatrix T = TranslationMatrixInverse(Location);
    FMatrix R = RotationMatrixInverse(Rotation);
    FMatrix S = ScaleMatrixInverse(Scale);
    return T * R * S;
}

/**
 * @brief 좌표계 기준변환: LHY+ -> UE(LHZ+, X-forward)
 * (x,y,z) -> (z,x,y) 로 순열 전환하는 행렬
 */
FMatrix FMatrix::BasisLHYToUE()
{
	// row-major, row-vector mul(p, M) 기준
	// [[0,0,1,0],
	//  [1,0,0,0],
	//  [0,1,0,0],
	//  [0,0,0,1]]
	return {
		0, 0, 1, 0,
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};
}

/**
 * @brief 좌표계 기준변환의 역행렬: UE(LHZ+, X-forward) -> LHY+
 * 직교 순열행렬의 역행렬은 전치행렬과 동일
 */
FMatrix FMatrix::BasisUEToLHY()
{
	// transpose of BasisLHYToUE
	return {
		0, 1, 0, 0,
		0, 0, 1, 0,
		1, 0, 0, 0,
		0, 0, 0, 1
	};
}

const FMatrix FMatrix::Identity = FMatrix(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
);

const FMatrix FMatrix::Zero = FMatrix();
