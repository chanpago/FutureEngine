#pragma once

struct FVector;
struct FMatrix;

/**
 * @brief Row-major, row-vector 규약에 맞춘 쿼터니언
 */
struct FQuat
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float W = 1.0f;

    FQuat() = default;
    FQuat(float InX, float InY, float InZ, float InW) : X(InX), Y(InY), Z(InZ), W(InW) {}

	static const FQuat Identity;
	static const FQuat Zero;

    static FQuat FromAxisAngle(const FVector& Axis, float Radians);

    /**
     * @brief XYZ(롤-피치-야우 순서 정의는 엔진 표준) 각 입력은 degree
     */
    static FQuat FromEulerXYZ(const FVector& Degrees);

    /**
     * @brief Degree Euler로 역변환 (UI 표기용)
     */
    static FVector ToEulerXYZ(const FQuat& Q);

    void Normalize();
    FQuat Conjugate() const { return {-X, -Y, -Z, W}; }
    FQuat Inverse() const;

    FQuat operator*(const FQuat& Rhs) const; // Hamilton product

    FVector RotateVector(const FVector& V) const; // v' = q * v * q^-1
};

/**
 * @brief 쿼터니언을 row-major 회전행렬로 변환
 */
FMatrix QuatToRotationMatrix(const FQuat& Q);
FMatrix QuatToRotationMatrixInverse(const FQuat& Q);

