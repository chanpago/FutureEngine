#pragma once
#include <immintrin.h>

static inline __m128 LoadRow(const float* r4) { return _mm_loadu_ps(r4); }
static inline __m128 LoadCol(const float m[4][4], int j)
{
	// 열 j를 [m00,m10,m20,m30] 순서로 로드
	return _mm_set_ps(m[3][j], m[2][j], m[1][j], m[0][j]);
}

static inline void DotRowByCols_4x(__m128 row, __m128 c0, __m128 c1, __m128 c2, __m128 c3, float* dst4)
{
#if defined(__AVX2__)
	__m128 d0 = _mm_dp_ps(row, c0, 0xF1);
	__m128 d1 = _mm_dp_ps(row, c1, 0xF1);
	__m128 d2 = _mm_dp_ps(row, c2, 0xF1);
	__m128 d3 = _mm_dp_ps(row, c3, 0xF1);
	dst4[0] = _mm_cvtss_f32(d0);
	dst4[1] = _mm_cvtss_f32(d1);
	dst4[2] = _mm_cvtss_f32(d2);
	dst4[3] = _mm_cvtss_f32(d3);
#else
	dst4[0] = dst4[1] = dst4[2] = dst4[3] = 0.0f;
	for (int k = 0;k < 4;++k) {
		dst4[0] += row.m128_f32[k] * c0.m128_f32[k];
		dst4[1] += row.m128_f32[k] * c1.m128_f32[k];
		dst4[2] += row.m128_f32[k] * c2.m128_f32[k];
		dst4[3] += row.m128_f32[k] * c3.m128_f32[k];
	}
#endif
}


struct FVector;
struct FVector4;

struct FMatrix
{
	/**
	* @brief 4x4 float 타입의 행렬
	*/
	float Data[4][4];


	/**
	* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
	*/
	FMatrix();

	/**
	* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
	*/
	FMatrix(
		float M00, float M01, float M02, float M03,
		float M10, float M11, float M12, float M13,
		float M20, float M21, float M22, float M23,
		float M30, float M31, float M32, float M33);

	/**
	 * @brief 전치행렬
	 */

	static FMatrix Transpose(const FMatrix& InOtherMatrix);


	/**
	* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
	*/
	FMatrix operator*(const FMatrix& InOtherMatrix) const;
	void operator*=(const FMatrix& InOtherMatrix);

	/**
	* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FMatrix TranslationMatrix(const FVector& InOtherVector);
	static FMatrix TranslationMatrixInverse(const FVector& InOtherVector);

	/**
	* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FMatrix ScaleMatrix(const FVector& InOtherVector);
	static FMatrix ScaleMatrixInverse(const FVector& InOtherVector);

	/**
	* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FMatrix RotationMatrix(const FVector& InOtherVector);
	static FMatrix RotationMatrixInverse(const FVector& InOtherVector);

	// Quaternion 기반 회전행렬 (row-major)
	static FMatrix RotationMatrix(const struct FQuat& Q);
	static FMatrix RotationMatrixInverse(const struct FQuat& Q);
	static FQuat ToQuat(const FMatrix& Mat);

	/**
	 * @brief Camera용 Rotation의 정보를 행렬로 변환하여 제공하는 함수
	 */
	static FMatrix RotationMatrixCamera(const FVector& InOtherVector);
	static FMatrix RotationMatrixInverseCamera(const FVector& InOtherVector);
	/**
	* @brief X의 회전 정보를 행렬로 변환
	*/
	static FMatrix RotationX(float Radian);

	/**
	* @brief Y의 회전 정보를 행렬로 변환
	*/
	static FMatrix RotationY(float Radian);

	/**
	* @brief Y의 회전 정보를 행렬로 변환
	*/
	static FMatrix RotationZ(float Radian);

	static FMatrix GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale);

	static FMatrix GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale);

	// Quaternion 버전 TRS 조합 (row-major, row-vector: I * S * R * T)
	static FMatrix GetModelMatrix(const FVector& Location, const struct FQuat& Rotation, const FVector& Scale);
	static FMatrix GetModelMatrixInverse(const FVector& Location, const struct FQuat& Rotation, const FVector& Scale);

	/**
	 * @brief LHY+ -> UE(LHZ+, X-forward) 기준변환 행렬과 그 역행렬
	 * (x,y,z) -> (z,x,y) 순열 전환. 직교행렬이므로 역행렬은 전치행렬과 동일.
	 */
	static FMatrix BasisLHYToUE();
	static FMatrix BasisUEToLHY();

	/**
	* @brief 항등행렬
	*/
	static const FMatrix Identity;

	/**
	 * @brief 영행렬
	 */
	static const FMatrix Zero;
};
