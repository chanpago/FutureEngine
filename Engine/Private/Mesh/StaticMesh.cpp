#include "pch.h"
#include "Mesh/StaticMesh.h"
#include "Render/Renderer/Renderer.h"
#include "Math/AABB.h"
#include "Mesh/Material.h"
#include "Math/Bvh.h"

IMPLEMENT_CLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh()
{
	CalculateLocalAABB();
}

UStaticMesh::UStaticMesh(FStaticMesh* InStaticMeshAsset)
{
	SetStaticMeshAsset(InStaticMeshAsset);
	CalculateLocalAABB(); 
}

UStaticMesh::~UStaticMesh()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
	}
	if (IndexBuffer)
	{
		IndexBuffer->Release();
	}
}

FStaticMesh* UStaticMesh::GetStaticMeshAsset()
{
	return StaticMeshAsset;
}

FAABB UStaticMesh::GetLocalAABB() const
{
	return AABB;
}
bool UStaticMesh::IsRayCollided(const FRay& ModelRay, const TArray<FVector>& Vertices, const TArray<uint32>& Indices) const
{
	return Bvh->IsRayCollided(ModelRay, Vertices, Indices);
}
//
//const FObjMaterialInfo* UStaticMesh::GetMaterialInfo(const FString& MtlName) const
//{
//	return MaterialInfo->GetMaterialInfo(MtlName);
//}
//
//const FString& UStaticMesh::GetKdTextureFilePath(const FString& MtlName) const
//{
//	return MaterialInfo->GetKdTextureFilePath(MtlName);
//}
//
//const FString& UStaticMesh::GetKsTextureFilePath(const FString& MtlName) const
//{
//	return MaterialInfo->GetKsTextureFilePath(MtlName);
//}
//
//const FString& UStaticMesh::GetBumpTextureFilePath(const FString& MtlName) const
//{
//	return MaterialInfo->GetBumpTextureFilePath(MtlName);
//}

void UStaticMesh::SetStaticMeshAsset(FStaticMesh* InStaticMeshAsset)
{
	URenderer& Renderer = URenderer::GetInstance();
	if (!InStaticMeshAsset)
	{
		UE_LOG("MeshAsset이 유효하지 않아 SetStaticMeshAsset이 반환되었습니다");
		return;
	}
	if (VertexBuffer)
	{
		VertexBuffer->Release();
	}
	if (IndexBuffer)
	{
		IndexBuffer->Release();
	}

	StaticMeshAsset = InStaticMeshAsset;
	VertexBuffer = Renderer.CreateVertexBuffer(InStaticMeshAsset->Vertices);
	IndexBuffer = Renderer.CreateIndexBuffer(InStaticMeshAsset->Indices);
	IndexNum = InStaticMeshAsset->IndexNum;

	CalculateLocalAABB();

}

void UStaticMesh::SetBvh()
{
	if (!Bvh)
	{
		VertexPosition.reserve(StaticMeshAsset->Vertices.Num());
		for (int Index = 0; Index < StaticMeshAsset->Vertices.Num(); Index++)
		{
			VertexPosition.Add(StaticMeshAsset->Vertices[Index].Position);
		}
		Bvh = std::make_unique<FBvh>(VertexPosition, StaticMeshAsset->Indices);
	}
}

//void UStaticMesh::SetMaterialInfo(TMap<FString, FObjMaterialInfo*>* InMaterialInfo)
//{
//	MaterialInfo->SetMaterialInfo(InMaterialInfo);
//}


void UStaticMesh::CalculateLocalAABB()
{
	if (!StaticMeshAsset)
		return;


	const TArray<FNormalVertex>& Vertices = StaticMeshAsset->Vertices;

	switch (PrimitiveType)
	{
	default:
		for (int Index = 0; Index < Vertices.Num();Index++)
		{
			AABB.AddPoint(Vertices[Index].Position);
		}break;
	case EPrimitiveType::Sphere:
	{
		float Radius = Vertices[0].Position.Length();
		AABB.Min = FVector(-Radius, -Radius, -Radius);
		AABB.Max = FVector(Radius, Radius, Radius);
	}
	}


}

