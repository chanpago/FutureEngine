#pragma once
#include "Core/Object.h"
#include "Math/AABB.h"

struct FBvh;
class UMaterial;

class UStaticMesh : public UObject
{
	DECLARE_CLASS(UStaticMesh, UObject)

public:
	UStaticMesh();
	UStaticMesh(FStaticMesh* InStaticMeshAsset);
	~UStaticMesh();

	const FString& GetAssetPathFileName() const { return StaticMeshAsset->PathFileName; }
	ID3D11Buffer* GetVertexBuffer() { return VertexBuffer; }
	ID3D11Buffer* GetIndexBuffer() { return IndexBuffer; }
	const TArray<FVector>& GetVertexPosition() const { return VertexPosition; }
	uint32 GetIndexNum() { return IndexNum; }
	FStaticMesh* GetStaticMeshAsset();
	FAABB GetLocalAABB() const;
	bool IsRayCollided(const FRay& ModelRay, const TArray<FVector>& Vertices, const TArray<uint32>& Indices) const;
	EPrimitiveType GetPrimitiveType() const { return PrimitiveType; }

	void SetStaticMeshAsset(FStaticMesh* InStaticMeshAsset);
	void SetPrimtiveType(EPrimitiveType Type) { PrimitiveType = Type; }
	void SetBvh();


private:
	void CalculateLocalAABB();

	FStaticMesh* StaticMeshAsset = nullptr;
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	TArray<FVector> VertexPosition;
	unique_ptr<FBvh> Bvh;
	uint32 IndexNum = 0;
	EPrimitiveType PrimitiveType = EPrimitiveType::None;
	FAABB AABB = FAABB();
};
