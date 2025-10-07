#include "pch.h"
#include "Vector.h"
#include "SIMDHelper.h"

/**
 * @brief FVector2 기본 생성자
 */
FVector2::FVector2()
	: X(0), Y(0)
{
}


/**
 * @brief FVector2의 멤버값을 Param으로 넘기는 생성자
 */
FVector2::FVector2(float InX, float InY)
	: X(InX), Y(InY)
{
}


/**
 * @brief FVector2를 Param으로 넘기는 생성자
 */
FVector2::FVector2(const FVector2& InOther)
	: X(InOther.X), Y(InOther.Y)
{
}

FVector2 FVector2::operator+(const FVector2& o) const {
#if defined(__AVX2__)
	__m128 a = load2(&X), b = load2(&o.X);
	__m128 r = _mm_add_ps(a, b);
	FVector2 out; store2(&out.X, r); return out;
#else
	return { X + o.X, Y + o.Y };
#endif
}

FVector2 FVector2::operator-(const FVector2& o) const {
#if defined(__AVX2__)
	__m128 a = load2(&X), b = load2(&o.X);
	__m128 r = _mm_sub_ps(a, b);
	FVector2 out; store2(&out.X, r); return out;
#else
	return { X - o.X, Y - o.Y };
#endif
}

FVector2 FVector2::operator*(float s) const {
#if defined(__AVX2__)
	__m128 a = load2(&X), k = _mm_set1_ps(s);
	__m128 r = _mm_mul_ps(a, k);
	FVector2 out; store2(&out.X, r); return out;
#else
	return { X * s, Y * s };
#endif
}

FVector2& FVector2::operator+=(const FVector2& o) {
#if defined(__AVX2__)
	__m128 a = load2(&X), b = load2(&o.X);
	__m128 r = _mm_add_ps(a, b);
	store2(&X, r); return *this;
#else
	X += o.X; Y += o.Y; return *this;
#endif
}

FVector2& FVector2::operator-=(const FVector2& o) {
#if defined(__AVX2__)
	__m128 a = load2(&X), b = load2(&o.X);
	__m128 r = _mm_sub_ps(a, b);
	store2(&X, r); return *this;
#else
	X -= o.X; Y -= o.Y; return *this;
#endif
}

FVector2& FVector2::operator*=(float s) {
#if defined(__AVX2__)
	__m128 a = load2(&X), k = _mm_set1_ps(s);
	__m128 r = _mm_mul_ps(a, k);
	store2(&X, r); return *this;
#else
	X *= s; Y *= s; return *this;
#endif
}


/**
 * @brief FVector 기본 생성자
 */
FVector::FVector()
	: X(0), Y(0), Z(0)
{
}


/**
 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
 */
FVector::FVector(float InX, float InY, float InZ)
	: X(InX), Y(InY), Z(InZ)
{
}


/**
 * @brief FVector를 Param으로 넘기는 생성자
 */
FVector::FVector(const FVector& InOther)
	: X(InOther.X), Y(InOther.Y), Z(InOther.Z)
{
}

void FVector::operator=(const FVector4& InOther)
{
	*this = FVector(InOther.X, InOther.Y, InOther.Z);
}


FVector FVector::operator+(const FVector& o) const {
#if defined(__AVX2__)
	__m128 a = load3(&X), b = load3(&o.X);
	__m128 r = _mm_add_ps(a, b);
	FVector out; store3(&out.X, r); return out;
#else
	return { X + o.X, Y + o.Y, Z + o.Z };
#endif
}

FVector FVector::operator-(const FVector& o) const {
#if defined(__AVX2__)
	__m128 a = load3(&X), b = load3(&o.X);
	__m128 r = _mm_sub_ps(a, b);
	FVector out; store3(&out.X, r); return out;
#else
	return { X - o.X, Y - o.Y, Z - o.Z };
#endif
}

FVector FVector::operator*(float s) const {
#if defined(__AVX2__)
	__m128 a = load3(&X), k = _mm_set1_ps(s);
	__m128 r = _mm_mul_ps(a, k);
	FVector out; store3(&out.X, r); return out;
#else
	return { X * s, Y * s, Z * s };
#endif
}

FVector FVector::operator*(const FVector& InOther) const
{
	return FVector(X * InOther.X, Y * InOther.Y, Z * InOther.Z);
}

FVector& FVector::operator+=(const FVector& o) {
#if defined(__AVX2__)
	__m128 a = load3(&X), b = load3(&o.X);
	__m128 r = _mm_add_ps(a, b);
	store3(&X, r); return *this;
#else
	X += o.X; Y += o.Y; Z += o.Z; return *this;
#endif
}

FVector& FVector::operator-=(const FVector& o) {
#if defined(__AVX2__)
	__m128 a = load3(&X), b = load3(&o.X);
	__m128 r = _mm_sub_ps(a, b);
	store3(&X, r); return *this;
#else
	X -= o.X; Y -= o.Y; Z -= o.Z; return *this;
#endif
}

FVector& FVector::operator*=(float s) {
#if defined(__AVX2__)
	__m128 a = load3(&X), k = _mm_set1_ps(s);
	__m128 r = _mm_mul_ps(a, k);
	store3(&X, r); return *this;
#else
	X *= s; Y *= s; Z *= s; return *this;
#endif
}

float FVector::Length() const
{
#if defined(__AVX2__)
	__m128 v = load3(&X);
	__m128 dp = _mm_dp_ps(v, v, 0x71);        // x^2+y^2+z^2 -> [s,0,0,0]
	__m128 s = _mm_sqrt_ss(dp);
	return _mm_cvtss_f32(s);
#else
	return sqrtf(X * X + Y * Y + Z * Z);
#endif
}

float FVector::LengthSquared() const
{
#if defined(__AVX2__)
	__m128 v = load3(&X);
	__m128 dp = _mm_dp_ps(v, v, 0x71);
	return _mm_cvtss_f32(dp);
#else
	return X * X + Y * Y + Z * Z;
#endif
}

float FVector::Dot(const FVector& OtherVector) const
{
#if defined(__AVX2__)
	const float a[4] = { X, Y, Z, 0.0f };
	const float b[4] = { OtherVector.X, OtherVector.Y, OtherVector.Z, 0.0f };
	return Dot3_AVX(a, b);
#else
	return (X * OtherVector.X) + (Y * OtherVector.Y) + (Z * OtherVector.Z);
#endif
}

FVector FVector::Cross(const FVector& OutVector) const
{
#if defined(__AVX2__)
	// a x b = (a_y b_z - a_z b_y, a_z b_x - a_x b_z, a_x b_y - a_y b_x)
	__m128 a = load3(&X);
	__m128 b = load3(&OutVector.X);
	// a_yzx, a_zxy
	__m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 a_zxy = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
	// b_zxy, b_yzx
	__m128 b_zxy = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
	__m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 c = _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
	FVector out; store3(&out.X, c); return out;
#else
	return FVector(
		Z * OutVector.Y - Y * OutVector.Z,
		X * OutVector.Z - Z * OutVector.X,
		Y * OutVector.X - X * OutVector.Y
	);
#endif
}

void FVector::Normalize()
{
#if defined(__AVX2__)
	__m128 v = load3(&X);
	__m128 dp = _mm_dp_ps(v, v, 0x7F);   // 스칼라 s, 나머지 브로드캐스트
	__m128 len = _mm_sqrt_ps(dp);
	// 0 나눗셈 방지용 epsilon
	__m128 eps = _mm_set1_ps(1e-8f);
	__m128 m = _mm_max_ps(len, eps);
	__m128 inv = _mm_div_ps(_mm_set1_ps(1.0f), m);
	__m128 r = _mm_mul_ps(v, inv);
	store3(&X, r);
#else
	float L = sqrt(LengthSquared());
	if (L > 0.00000001f) { X /= L; Y /= L; Z /= L; }
#endif
}


	/**
	 * @brief FVector 기본 생성자
	 */
FVector4::FVector4()
		: X(0), Y(0), Z(0), W(0)
{
}

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
FVector4::FVector4(const float InX, const float InY, const float InZ, const float InW)
		: X(InX), Y(InY), Z(InZ), W(InW)
{
}

FVector4::FVector4(const FVector& InOther, const float InW)
	:X(InOther.X), Y(InOther.Y), Z(InOther.Z), W(InW)
{
}


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
FVector4::FVector4(const FVector4& InOther)
		: X(InOther.X), Y(InOther.Y), Z(InOther.Z), W(InOther.W)
{
}


/**
 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
 */
FVector4 FVector4::operator+(const FVector4& OtherVector) const
{
#if defined(__AVX2__)
	__m128 a = _mm_loadu_ps(&X);
	__m128 b = _mm_loadu_ps(&OtherVector.X);
	__m128 r = _mm_add_ps(a, b);
	FVector4 out; _mm_storeu_ps(&out.X, r); return out;
#else
	return FVector4(X + OtherVector.X, Y + OtherVector.Y, Z + OtherVector.Z, W + OtherVector.W);
#endif
}

FVector4 FVector4::operator*(const FMatrix& Matrix) const
{
#if defined(__AVX2__)
	__m128 v = _mm_loadu_ps(&X); // [X,Y,Z,W]
	__m128 c0 = LoadCol(Matrix.Data, 0);
	__m128 c1 = LoadCol(Matrix.Data, 1);
	__m128 c2 = LoadCol(Matrix.Data, 2);
	__m128 c3 = LoadCol(Matrix.Data, 3);

	FVector4 out;
	out.X = _mm_cvtss_f32(_mm_dp_ps(v, c0, 0xF1));
	out.Y = _mm_cvtss_f32(_mm_dp_ps(v, c1, 0xF1));
	out.Z = _mm_cvtss_f32(_mm_dp_ps(v, c2, 0xF1));
	out.W = _mm_cvtss_f32(_mm_dp_ps(v, c3, 0xF1));
	return out;
#else
	FVector4 Result;
	Result.X = X * Matrix.Data[0][0] + Y * Matrix.Data[1][0] + Z * Matrix.Data[2][0] + W * Matrix.Data[3][0];
	Result.Y = X * Matrix.Data[0][1] + Y * Matrix.Data[1][1] + Z * Matrix.Data[2][1] + W * Matrix.Data[3][1];
	Result.Z = X * Matrix.Data[0][2] + Y * Matrix.Data[1][2] + Z * Matrix.Data[2][2] + W * Matrix.Data[3][2];
	Result.W = X * Matrix.Data[0][3] + Y * Matrix.Data[1][3] + Z * Matrix.Data[2][3] + W * Matrix.Data[3][3];
	return Result;
#endif
}
/**
 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
 */
FVector4 FVector4::operator-(const FVector4& OtherVector) const
{
#if defined(__AVX2__)
	__m128 a = _mm_loadu_ps(&X), b = _mm_loadu_ps(&OtherVector.X);
	__m128 r = _mm_sub_ps(a, b);
	FVector4 out; _mm_storeu_ps(&out.X, r); return out;
#else
	return FVector4(X - OtherVector.X, Y - OtherVector.Y, Z - OtherVector.Z, W - OtherVector.W);
#endif
}

/**
 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
 */
FVector4 FVector4::operator*(const float Ratio) const
{
#if defined(__AVX2__)
#pragma message("AVX2 path compiled")
	static volatile int hit_avx2 = (printf("AVX2 path\n"), 0);

	__m128 a = _mm_loadu_ps(&X);
	__m128 s = _mm_set1_ps(Ratio);
	__m128 r = _mm_mul_ps(a, s);
	FVector4 out; _mm_storeu_ps(&out.X, r); return out;
#else
	return FVector4(X * Ratio, Y * Ratio, Z * Ratio, W * Ratio);
#endif
}


/**
 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
 */
void FVector4::operator+=(const FVector4& OtherVector)
{
#if defined(__AVX2__)
	__m128 a = _mm_loadu_ps(&X), b = _mm_loadu_ps(&OtherVector.X);
	__m128 r = _mm_add_ps(a, b);
	_mm_storeu_ps(&X, r);
#else
	X += OtherVector.X; Y += OtherVector.Y; Z += OtherVector.Z; W += OtherVector.W;
#endif
}

/**
 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
 */
void FVector4::operator-=(const FVector4& OtherVector)
{
#if defined(__AVX2__)
	__m128 a = _mm_loadu_ps(&X), b = _mm_loadu_ps(&OtherVector.X);
	__m128 r = _mm_sub_ps(a, b);
	_mm_storeu_ps(&X, r);
#else
	X -= OtherVector.X; Y -= OtherVector.Y; Z -= OtherVector.Z; W -= OtherVector.W;
#endif
}

/**
 * @brief 자신의 벡터에 배율을 곱하는 함수
 */
void FVector4::operator*=(const float Ratio)
{
#if defined(__AVX2__)
	__m128 a = _mm_loadu_ps(&X), k = _mm_set1_ps(Ratio);
	__m128 r = _mm_mul_ps(a, k);
	_mm_storeu_ps(&X, r);
#else
	X *= Ratio; Y *= Ratio; Z *= Ratio; W *= Ratio;
#endif
}

// FVector static const 멤버 변수 정의
const FVector FVector::ZeroVector = FVector(0.0f, 0.0f, 0.0f);
const FVector FVector::OneVector = FVector(1.0f, 1.0f, 1.0f);

// FVector4 static const 멤버 변수 정의
const FVector4 FVector4::ZeroVector = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
const FVector4 FVector4::OneVector = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

