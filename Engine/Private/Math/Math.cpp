#include "pch.h"
#include "Math/Math.h"
bool FMath::IsRayCollidWithAABB(const FRay& Ray, const FAABB& AABB, float& CollisionTime)
{
	//Ray.Origin.X + t*Ray.Direction.X가 AABB.min.X, max.X 사이일때 t값 범위가 Y일때 t z일때 t값 범위와 겹쳐야함
	//그러려면 min t값들의 max값이 max t값들의 min값보다 작거나 같아야 함.


	if (std::abs(Ray.Direction.X) < 0.001f && (Ray.Origin.X < AABB.Min.X  || Ray.Origin.X > AABB.Max.X))
	{
		return false;
	}
	if (std::abs(Ray.Direction.Z) < 0.001f && (Ray.Origin.Z < AABB.Min.Z || Ray.Origin.Z > AABB.Max.Z))
	{
		return false;
	}
	if (std::abs(Ray.Direction.Y) < 0.001f && (Ray.Origin.Y < AABB.Min.Y || Ray.Origin.Y > AABB.Max.Y))
	{
		return false;
	}
	float DivDirectionX = 1 / Ray.Direction.X;
	float DivDirectionY = 1 / Ray.Direction.Y;
	float DivDirectionZ = 1 / Ray.Direction.Z;
	float MinTimeMax = (AABB.Min.X - Ray.Origin.X) * DivDirectionX;
	float MaxTimeMin = (AABB.Max.X - Ray.Origin.X) * DivDirectionX;

	if (MinTimeMax > MaxTimeMin)
	{
		std::swap(MinTimeMax, MaxTimeMin);
	}

	float MinTimeY = (AABB.Min.Y - Ray.Origin.Y) * DivDirectionY;
	float MaxTimeY = (AABB.Max.Y - Ray.Origin.Y) * DivDirectionY;

	if (MinTimeY > MaxTimeY)
	{
		std::swap(MinTimeY, MaxTimeY);
	}
	float MinTimeZ = (AABB.Min.Z - Ray.Origin.Z) * DivDirectionZ;
	float MaxTimeZ = (AABB.Max.Z - Ray.Origin.Z) * DivDirectionZ;
	if (MinTimeZ > MaxTimeZ)
	{
		std::swap(MinTimeZ, MaxTimeZ);
	}
	MinTimeMax = std::fmax(MinTimeMax, MinTimeY);
	MinTimeMax = std::fmax(MinTimeMax, MinTimeZ);

	MaxTimeMin = std::fmin(MaxTimeMin, MaxTimeY);
	MaxTimeMin = std::fmin(MaxTimeMin, MaxTimeZ);


	
	if (MinTimeMax > MaxTimeMin)
	{
		CollisionTime = -1;

		return false;
	}
	else
	{
		CollisionTime = MinTimeMax;
		return true;
	}
}

bool FMath::IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, float* Distance)
{


	//삼각형 내의 점은 E1*V + E2*U + Vertex1.Position으로 표현 가능( 0<= U + V <=1,  Y>=0, V>=0 )
	//Ray.Direction * T + Ray.Origin = E1*V + E2*U + Vertex1.Position을 만족하는 T U V값을 구해야 함.
	//[E1 E2 RayDirection][V U T] = [RayOrigin-Vertex1.Position]에서 cramer's rule을 이용해서 T U V값을 구하고
	//U V값이 저 위의 조건을 만족하고 T값이 카메라의 near값 이상이어야 함.
	FVector RayDirection{ Ray.Direction.X, Ray.Direction.Y, Ray.Direction.Z };
	FVector RayOrigin{ Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z };
	FVector E1 = Vertex2 - Vertex1;
	FVector E2 = Vertex3 - Vertex1;
	FVector Result = (RayOrigin - Vertex1); //[E1 E2 -RayDirection]x = [RayOrigin - Vertex1.Position] 의 result임.


	FVector CrossE2Ray = E2.Cross(RayDirection);
	FVector CrossE1Result = E1.Cross(Result);

	float Determinant = E1.Dot(CrossE2Ray);
	float DeterminantDiv = 1 / Determinant;

	float NoInverse = 0.0001f; //0.0001이하면 determinant가 0이라고 판단=>역행렬 존재 X
	if (abs(Determinant) <= NoInverse)
	{
		return false;
	}


	float V = Result.Dot(CrossE2Ray) * DeterminantDiv; //cramer's rule로 해를 구했음. 이게 0미만 1초과면 충돌하지 않음.

	if (V < 0 || V > 1)
	{
		return false;
	}

	float U = RayDirection.Dot(CrossE1Result) * DeterminantDiv;
	if (U < 0 || U + V > 1)
	{
		return false;
	}


	float T = E2.Dot(CrossE1Result) * DeterminantDiv;

	FVector HitPoint = RayOrigin + RayDirection * T; //모델 좌표계에서의 충돌점
	FVector4 HitPoint4{ HitPoint.X, HitPoint.Y, HitPoint.Z, 1 };
	FVector4 DistanceVec = HitPoint4 - Ray.Origin;
	if (T > 0)
	{
		*Distance = DistanceVec.Length();
		return true;
	}
	return false;
}



float FMath::Dist2(const FVector& P0, const FVector& P1)
{
	return (P1-P0).Length();

}

inline float Dist2ToAABBNear(const FVector& p, const FAABB& b)
{
	auto clampf = [](float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); };
	const float cx = clampf(p.X, b.Min.X, b.Max.X);
	const float cy = clampf(p.Y, b.Min.Y, b.Max.Y);
	const float cz = clampf(p.Z, b.Min.Z, b.Max.Z);
	const float dx = p.X - cx, dy = p.Y - cy, dz = p.Z - cz;
	return dx * dx + dy * dy + dz * dz;
}
