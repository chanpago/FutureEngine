#include "pch.h"
#include "Components/StaticMeshComponent.h"
#include "Level/Level.h"
#include "Math/AABB.h"
#include "Mesh/Material.h"
#include "Utility/ObjManager.h"
#include "Editor/ObjectPicker.h"

#include <cstring>

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InStaticMesh)
{
	StaticMesh = InStaticMesh;

	if (!StaticMesh)
	{
		MaterialList.SetNum(0);
		WorldBound = FAABB();
		CachedBoundsMatrix = FMatrix::Zero;
		return;
	}

	const FStaticMesh* StaticMeshAsset = StaticMesh->GetStaticMeshAsset();
	if (!StaticMeshAsset)
	{
		MaterialList.SetNum(0);
		WorldBound = FAABB();
		CachedBoundsMatrix = FMatrix::Zero;
		return;
	}

	MaterialList.SetNum(StaticMeshAsset->Sections.Num());

	for (int Index = 0; Index < StaticMeshAsset->Sections.Num(); Index++)
	{
		MaterialList[Index] = FObjManager::LoadMaterial(StaticMeshAsset->Sections[Index].MaterialName);
	}

	WorldBound = FAABB();
	CachedBoundsMatrix = FMatrix::Zero;
}

FAABB UStaticMeshComponent::GetWorldBounds() const
{
	if (!StaticMesh)
	{
		return FAABB();
	}

	const FMatrix WorldMatrix = GetWorldTransformMatrix();
	const bool bMatrixChanged = std::memcmp(&CachedBoundsMatrix, &WorldMatrix, sizeof(FMatrix)) != 0;

	if (!bMatrixChanged && WorldBound.IsValid())
	{
		return WorldBound;
	}

	switch (StaticMesh->GetPrimitiveType())
	{
	case EPrimitiveType::Sphere:
	{
		const float ScaleX = FVector(WorldMatrix.Data[0][0], WorldMatrix.Data[1][0], WorldMatrix.Data[2][0]).Length();
		const float ScaleY = FVector(WorldMatrix.Data[0][1], WorldMatrix.Data[1][1], WorldMatrix.Data[2][1]).Length();
		const float ScaleZ = FVector(WorldMatrix.Data[0][2], WorldMatrix.Data[1][2], WorldMatrix.Data[2][2]).Length();
		const FVector Position(WorldMatrix.Data[3][0], WorldMatrix.Data[3][1], WorldMatrix.Data[3][2]);
		WorldBound = StaticMesh->GetLocalAABB().TransformBy(
			FMatrix::GetModelMatrix(Position, FQuat(), FVector(ScaleX, ScaleY, ScaleZ)));
		break;
	}
	default:
		WorldBound = StaticMesh->GetLocalAABB().TransformBy(WorldMatrix);
		break;
	}

	CachedBoundsMatrix = WorldMatrix;
	return WorldBound;
}
UStaticMeshComponent::UStaticMeshComponent()
{
	ComponentType = EComponentType::StaticMesh;
}

bool UStaticMeshComponent::IsRayCollided(const FRay& WorldRay, float& ShortestDistance) const
{
	if (!FMath::IsRayCollidWithAABB(WorldRay, this->GetWorldBounds(), ShortestDistance))
	{
		return false;
	}
	FRay ModelRay = UObjectPicker::GetModelRay(WorldRay, this);
	FStaticMesh* StaticMeshAsset = StaticMesh->GetStaticMeshAsset();

	const TArray<uint32> Indices = StaticMeshAsset->Indices;
	const TArray<FVector>& Vertices = StaticMesh->GetVertexPosition();
	
	return StaticMesh->IsRayCollided(ModelRay, Vertices, Indices);
}

void UStaticMeshComponent::DuplicateSubObjects()
{

}

UObject* UStaticMeshComponent::Duplicate()
{ 
	//Super::Duplicate();

	UStaticMeshComponent* Src = static_cast<UStaticMeshComponent*>(GetClass()->CreateDefaultObject());

	Src->CopyShallow(this);
	Src->DuplicateSubObjects();

	return Src;
}

void UStaticMeshComponent::CopyShallow(UObject* Src)
{
	Super::CopyShallow(Src);

	const UStaticMeshComponent* BaseStaticMesh = static_cast<const UStaticMeshComponent*>(Src);

	// 있어야 Octree에서 오류가 안생김
	this->SetStaticMesh(const_cast<UStaticMeshComponent*>(BaseStaticMesh)->GetStaticMesh());
	this->WorldBound = BaseStaticMesh->WorldBound;
	this->CachedBoundsMatrix = BaseStaticMesh->CachedBoundsMatrix;
	const TArray<UMaterial*>& Mats = const_cast<UStaticMeshComponent*>(BaseStaticMesh)->GetMaterialList();
	for (int i = 0; i < Mats.Num(); ++i)
	{
		this->SetMaterial(Mats[i], i);
	} 
}

//TODO: Serialize 구현
//void UStaticMeshComponent::Serialize(bool bIsLoading, json::JSON Handle)
//{
//	//부모 객체부터 Serialize가 필요한데 일단 패스
//	//Super::Serialize(bIsLoading, Handle);
//
//	/*if (bIsLoading)
//	{
//		FString assetName;
//		Handle << "ObjStaticMeshAsset" << assetName;
//		StaticMesh = FObjManager::LoadObjStaticMesh(assetName);
//	}
//	else
//	{
//		FString assetName = StaticMesh->GetAssetPathFileName();
//		Handle << "ObjStaticMeshAsset" << assetName;
//	}*/
//}


