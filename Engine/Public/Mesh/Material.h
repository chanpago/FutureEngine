#pragma once
#include "Core/Object.h"

class UMaterial : public UObject
{
	DECLARE_CLASS(UMaterial, UObject)

public:
	

	void SetMaterialInfo(const FObjMaterialInfo& InFObjMaterial);

	

	const FObjMaterialInfo& GetMaterialInfo() const { return MaterialInfo; }
	const FString& GetMaterialName() const { return MaterialInfo.MaterialName; }
	const FString& GetKdTextureFilePath() const { return MaterialInfo.Map_Kd; }
	const FString& GetKsTextureFilePath() const { return MaterialInfo.Map_Ks; }
	const FString& GetBumpTextureFilePath() const { return MaterialInfo.Map_bump; }

private:
	UMaterial();
	UMaterial(const FObjMaterialInfo& InFObjMaterial);
	~UMaterial();

	friend class FObjManager;
	friend class FMtlParser;
	FObjMaterialInfo MaterialInfo;
};
