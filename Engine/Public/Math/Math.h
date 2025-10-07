#pragma once

struct FRay;
struct FAABB;

struct FMath
{
	static bool IsRayCollidWithAABB(const FRay& WorldRay, const FAABB& AABB, float& CollisionTime);
	static bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, float* Distance);
	static float Dist2(const FVector& P0, const FVector& P1);

};
