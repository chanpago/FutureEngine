#pragma once

#include "Core/Object.h"

struct FAABB
{
public:
	FVector Min;
	FVector Max;

	FAABB();
	FAABB(const FVector& InMin, const FVector& InMax);
	FAABB(const FAABB& Other);

	void Reset();
	void AddPoint(const FVector& Point);
	void AddAABB(const FAABB& Other);

	FVector GetCenter() const;
	FVector GetExtent() const;
	FVector GetSize() const;
	float GetVolume() const;
	float GetSurfaceArea() const;

	bool IsValid() const;
	bool Contains(const FVector& Point) const;
	bool Contains(const FAABB& Bound)	const;
	bool Intersects(const FAABB& Other) const;

	FAABB TransformBy(const FMatrix& Transform) const;

	void ExpandBy(float Amount);
	void ExpandBy(const FVector& Amount);
	void ScaleBy(float Amount);

	FAABB& operator+=(const FVector& Point);
	FAABB& operator+=(const FAABB& Other);
	FAABB operator+(const FAABB& Other) const;

	bool operator==(const FAABB& Other) const;
	bool operator!=(const FAABB& Other) const;

private:
	static const float SMALL_NUMBER;
};
