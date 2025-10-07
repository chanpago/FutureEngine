#include "pch.h"
#include "Math/AABB.h"

const float FAABB::SMALL_NUMBER = 1e-6f;

FAABB::FAABB()
	: Min(FLT_MAX, FLT_MAX, FLT_MAX)
	, Max(-FLT_MAX, -FLT_MAX, -FLT_MAX)
{
}

FAABB::FAABB(const FVector& InMin, const FVector& InMax)
	: Min(InMin)
	, Max(InMax)
{
}

FAABB::FAABB(const FAABB& Other)
	: Min(Other.Min)
	, Max(Other.Max)
{
}

void FAABB::Reset()
{
	Min = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
	Max = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

void FAABB::AddPoint(const FVector& Point)
{
	Min.X = std::min(Min.X, Point.X);
	Min.Y = std::min(Min.Y, Point.Y);
	Min.Z = std::min(Min.Z, Point.Z);

	Max.X = std::max(Max.X, Point.X);
	Max.Y = std::max(Max.Y, Point.Y);
	Max.Z = std::max(Max.Z, Point.Z);
}

void FAABB::AddAABB(const FAABB& Other)
{
	if (!Other.IsValid()) return;

	AddPoint(Other.Min);
	AddPoint(Other.Max);
}

FVector FAABB::GetCenter() const
{
	return (Min + Max) * 0.5f;
}

FVector FAABB::GetExtent() const
{
	return (Max - Min) * 0.5f;
}

FVector FAABB::GetSize() const
{
	return Max - Min;
}

float FAABB::GetVolume() const
{
	if (!IsValid()) return 0.0f;

	FVector Size = GetSize();
	return Size.X * Size.Y * Size.Z;
}

float FAABB::GetSurfaceArea() const
{
	if (!IsValid()) return 0.0f;

	FVector Size = GetSize();
	return 2.0f * (Size.X * Size.Y + Size.Y * Size.Z + Size.Z * Size.X);
}

bool FAABB::IsValid() const
{
	return Min.X <= Max.X && Min.Y <= Max.Y && Min.Z <= Max.Z;
}

bool FAABB::Contains(const FVector& Point) const
{
	return Point.X >= Min.X && Point.X <= Max.X &&
		   Point.Y >= Min.Y && Point.Y <= Max.Y &&
		   Point.Z >= Min.Z && Point.Z <= Max.Z;
}

bool FAABB::Contains(const FAABB& Bound) const
{
	return Contains(Bound.Min) && Contains(Bound.Max);
}

bool FAABB::Intersects(const FAABB& Other) const
{
	if (!IsValid() || !Other.IsValid()) return false;

	return Min.X <= Other.Max.X && Max.X >= Other.Min.X &&
		   Min.Y <= Other.Max.Y && Max.Y >= Other.Min.Y &&
		   Min.Z <= Other.Max.Z && Max.Z >= Other.Min.Z;
}

FAABB FAABB::TransformBy(const FMatrix& Transform) const
{
	if (!IsValid()) return FAABB();

	FAABB Result;

	FVector Corners[8] = {
		FVector(Min.X, Min.Y, Min.Z),
		FVector(Min.X, Min.Y, Max.Z),
		FVector(Min.X, Max.Y, Min.Z),
		FVector(Min.X, Max.Y, Max.Z),
		FVector(Max.X, Min.Y, Min.Z),
		FVector(Max.X, Min.Y, Max.Z),
		FVector(Max.X, Max.Y, Min.Z),
		FVector(Max.X, Max.Y, Max.Z)
	};

	for (int32 i = 0; i < 8; ++i)
	{
		FVector4 TransformedPoint = FVector4(Corners[i].X, Corners[i].Y, Corners[i].Z, 1.0f) * Transform;
		Result.AddPoint(FVector(TransformedPoint.X, TransformedPoint.Y, TransformedPoint.Z));
	}

	return Result;
}

void FAABB::ExpandBy(float Amount)
{
	if (!IsValid()) return;

	FVector Expansion(Amount, Amount, Amount);
	Min -= Expansion;
	Max += Expansion;
}

void FAABB::ExpandBy(const FVector& Amount)
{
	if (!IsValid()) return;

	Min -= Amount;
	Max += Amount;
}

void FAABB::ScaleBy(float Amount)
{
	if (!IsValid()) return;
	FVector Center = (Min + Max) * (1/2.0f);
	FVector Extents = (Max - Min) *(1/ 2.0f);


	FVector ScaledExtents = Extents * Amount;

	Min = Center - ScaledExtents;
	Max = Center + ScaledExtents;
}

FAABB& FAABB::operator+=(const FVector& Point)
{
	AddPoint(Point);
	return *this;
}

FAABB& FAABB::operator+=(const FAABB& Other)
{
	AddAABB(Other);
	return *this;
}

FAABB FAABB::operator+(const FAABB& Other) const
{
	FAABB Result(*this);
	Result.AddAABB(Other);
	return Result;
}

bool FAABB::operator==(const FAABB& Other) const
{
	return (Min.X == Other.Min.X && Min.Y == Other.Min.Y && Min.Z == Other.Min.Z &&
		    Max.X == Other.Max.X && Max.Y == Other.Max.Y && Max.Z == Other.Max.Z);
}

bool FAABB::operator!=(const FAABB& Other) const
{
	return !(*this == Other);
}
