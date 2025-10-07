#pragma once
#include "Components/MeshComponent.h"
#include "Mesh/StaticMesh.h"
#include "Global/Name.h"
#include "Global/Matrix.h"


class UStaticMesh;
class UMaterial;
class UStaticMeshComponent : public UMeshComponent
{
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)
public:	

	UStaticMeshComponent();
	//StaticMesh가 구현되면 주석 해제(09/19 13:05)
	//자식 StaticMeshComponent가 본인 타입에 맞는 렌더 리스트에 알아서 추가
	void AddToRenderList(ULevel* Level) override {};
	bool IsRayCollided(const FRay& WorldRay, float& ShortestDistance) const override;
	FAABB GetWorldBounds() const override;
	///////////////////////////////////////////////////

	FString GetStaticMeshName() const { if (StaticMesh) return StaticMesh->GetAssetPathFileName(); else return FString(); }
	UStaticMesh* GetStaticMesh() { return StaticMesh; }
	const UMaterial* GetMaterial(int Index) const { if (Index < MaterialList.Num() && Index >= 0) return MaterialList[Index]; else return nullptr; }
	const TArray<UMaterial*>& GetMaterialList(){ return MaterialList; }

	void SetMaterial(UMaterial* Material, int Index) { if (Index < MaterialList.Num() && Index>=0) MaterialList[Index] = Material; else return; }
	void SetStaticMesh(UStaticMesh* InStaticMesh);
	   
	//TODO: Serialize 구현
	//StaticMesh에셋을 Json에 저장하거나 로드하는 함수
	//bIsLoading이 true : 로드 false : 저장
	//void Serialize(bool bIsLoading, json::JSON Handle);

	//PIE
	virtual void DuplicateSubObjects() override;
	virtual UObject* Duplicate() override; 
	virtual void CopyShallow(UObject* Src) override;

private:

	UStaticMesh* StaticMesh = nullptr;
	TArray<UMaterial*> MaterialList;
	mutable FAABB WorldBound{ FAABB() };
	mutable FMatrix CachedBoundsMatrix{ FMatrix::Zero };
};

