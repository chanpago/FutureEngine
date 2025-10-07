#pragma once
struct FMatrix;

// VectorSimd.h (분리 권장)
#pragma once
#include <immintrin.h>

// 4원수 로드/스토어 헬퍼 (row-major float[4])
static inline __m128 LoadVec4(const float* p) { return _mm_loadu_ps(p); }
static inline void    StoreVec4(float* p, __m128 v) { _mm_storeu_ps(p, v); }
static inline __m128  Splat(float s) { return _mm_set1_ps(s); }
static inline __m128  Set4(float x, float y, float z, float w) { return _mm_set_ps(w, z, y, x); } // XYZW -> lane WZYX

// 3D dot (W=0 가정), 4D dot
static inline float Dot3_AVX(const float* a, const float* b)
{
#if defined(__AVX2__)
	__m128 va = _mm_set_ps(0.0f, a[2], a[1], a[0]);
	__m128 vb = _mm_set_ps(0.0f, b[2], b[1], b[0]);
	// 0x71 = (maskL=0001, maskH=0111) -> XYZ만 곱/합, 결과는 하위 스칼라
	__m128 d = _mm_dp_ps(va, vb, 0x71);
	return _mm_cvtss_f32(d);
#else
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
#endif
}

static inline float Dot4_AVX(const float* a, const float* b)
{
#if defined(__AVX2__)
	__m128 va = _mm_loadu_ps(a);
	__m128 vb = _mm_loadu_ps(b);
	// 0xF1 = XYZW 모두 곱/합, 결과는 하위 스칼라
	__m128 d = _mm_dp_ps(va, vb, 0xF1);
	return _mm_cvtss_f32(d);
#else
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
#endif
}

struct FVector2
{
	float X;
	float Y;
	/**
	 * @brief FVector2 기본 생성자
	 */
	FVector2();


	/**
	 * @brief FVector2의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector2(float InX, float InY);


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector2(const FVector2& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector2 operator+(const FVector2& InOther) const;

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector2 operator-(const FVector2& InOther) const;

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 백테를 반환하는 함수
	 */
	FVector2 operator*(const float Ratio) const;

	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	FVector2& operator+=(const FVector2& InOther);

	/**
	 * @brief 자신의 벡터에서 다른 벡터를 감산하는 함수
	 */
	FVector2& operator-=(const FVector2& InOther);

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 뒤 자신을 반환
	 */
	FVector2& operator*=(const float Ratio);

	/**
	 * @brief 자신의 벡터의 각 성분의 부호를 반전한 값을 반환
	 */
	inline FVector2 operator-() const { return FVector2(-X, -Y); }
};


struct FVector
{
	float X;
	float Y;
	float Z;

	/**
	 * @brief FVector 기본 생성자
	 */
	FVector();


	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector(float InX, float InY, float InZ);


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector(const FVector& InOther);

	void operator=(const FVector4& InOther);
	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector operator+(const FVector& InOther) const;

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector operator-(const FVector& InOther) const;

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 백테를 반환하는 함수
	 */
	FVector operator*(const float Ratio) const;

	FVector operator*(const FVector& InOther) const;

	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	FVector& operator+=(const FVector& InOther);

	/**
	 * @brief 자신의 벡터에서 다른 벡터를 감산하는 함수
	 */
	FVector& operator-=(const FVector& InOther);

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 뒤 자신을 반환
	 */
	FVector& operator*=(const float Ratio);

	/**
	 * @brief 자신의 벡터의 각 성분의 부호를 반전한 값을 반환
	 */
	inline FVector operator-() const { return FVector(-X, -Y, -Z); }


	const float& operator[](int32 Index) const
	{
		assert(Index >= 0 && Index < 3);
		switch (Index)
		{
		case 0: return X;
		case 1: return Y;
		default: return Z;
		}
	}


	/**
	 * @brief 벡터의 길이 연산 함수
	 * @return 벡터의 길이
	 */
	float Length() const;

	/**
	 * @brief 자신의 벡터의 각 성분을 제곱하여 더한 값을 반환하는 함수 (루트 사용 X)
	 */
	float LengthSquared() const;

	/**
	 * @brief 두 벡터를 내적하여 결과의 스칼라 값을 반환하는 함수
	 */
	float Dot(const FVector& OtherVector) const;

    /**
     * @brief 두 벡터의 외적을 반환 (this × other)
     * 표준 정의(this × other)를 따릅니다. 좌표계의 handedness는
     * 사용처의 기준(행렬/축 정의)에 의해 결정됩니다.
     */
    FVector Cross(const FVector& OtherVector) const;


	/**
	 * @brief 단위 벡터로 변경하는 함수
	 */
	void Normalize();


	bool operator==(const FVector& vector) const
	{
		return (std::abs(X - vector.X) <= 0.001f) && (std::abs(Y - vector.Y) <=0.001f) && (std::abs(Z -vector.Z) <=0.001f);
	}

	/**
	 * @brief 각도를 라디안으로 변환한 값을 반환하는 함수
	 */
	inline static float GetDegreeToRadian(const float Degree) { return (Degree * Pi) / 180.f; }
	inline static FVector GetDegreeToRadian(const FVector& Rotation)
	{
		return FVector{ (Rotation.X * Pi) / 180.f, (Rotation.Y * Pi) / 180.f, (Rotation.Z * Pi) / 180.f };
	}
	/**
	 * @brief 라디안를 각도로 변환한 값을 반환하는 함수
	 */
	inline static float GetRadianToDegree(const float Radian) { return (Radian * 180.f) / Pi; }

	/**
	 * @brief 제로 벡터 (0, 0, 0) - 전역 참조 변수
	 */
	static const FVector ZeroVector;

	/**
	 * @brief 단위 벡터 (1, 1, 1) - 전역 참조 변수
	 */
	static const FVector OneVector;
};


struct FVector4
{
	float X;
	float Y;
	float Z;
	float W;

	/**
	 * @brief FVector 기본 생성자
	 */
	FVector4();

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector4(const float InX, const float InY, const float InZ, const float InW);


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector4(const FVector& InOther, const float InW);


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector4(const FVector4& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator+(const FVector4& OtherVector) const;

	/**
	 * @brief 벡터와 행렬곱
	 */
	FVector4 operator*(const FMatrix& Matrix) const;
	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator-(const FVector4& OtherVector) const;

	/**
	 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
	 */
	FVector4 operator*(const float Ratio) const;


	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	void operator+=(const FVector4& OtherVector);

	/**
	 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
	 */
	void operator-=(const FVector4& OtherVector);

	/**
	 * @brief 자신의 벡터에 배율을 곱하는 함수
	 */
	void operator*=(const float Ratio);


	inline float Length() const
	{
		return sqrtf(X * X + Y * Y + Z * Z + W * W);
	}

	inline void Normalize()
	{
		float Mag = this->Length();
		X /= Mag;
		Y /= Mag;
		Z /= Mag;
		W /= Mag;
	}


	/**
	 * @brief W성분 무시하고 dot product 진행하는 함수
	 */
	inline float Dot3(const FVector4& OtherVector) const
	{
		#if defined(__AVX2__)
				const float a[4] = { X, Y, Z, 0.0f };
				const float b[4] = { OtherVector.X, OtherVector.Y, OtherVector.Z, 0.0f };
				return Dot3_AVX(a, b);
		#else
				return X * OtherVector.X + Y * OtherVector.Y + Z * OtherVector.Z;
		#endif
	}
	inline float Dot3(const FVector& OtherVector) const
	{
		return X * OtherVector.X + Y * OtherVector.Y + Z * OtherVector.Z;
	}

	bool operator==(const FVector4& color) const
	{
		return (X == color.X) && (Y == color.Y) && (Z == color.Z) && (W == color.W);
	}

	FVector ToVector() const
	{
		return FVector(X, Y, Z);
	}

	/**
	 * @brief 제로 벡터 (0, 0, 0, 0) - 전역 참조 변수
	 */
	static const FVector4 ZeroVector;

	/**
	 * @brief 단위 벡터 (1, 1, 1, 1) - 전역 참조 변수
	 */
	static const FVector4 OneVector;

};
