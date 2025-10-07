#pragma once


struct FObjMaterialInfo;
class UMaterial;

class FMtlParser
{
public:
	FMtlParser() {};
	FMtlParser(TMap<FString, UMaterial*>* InMaterials);
	~FMtlParser() {};
	bool ParseMtl(const FString& PathFileName);

private:
	TMap<FString, UMaterial*>* Materials = nullptr;
};

void ParseToBinFormat(const FString& PathFileName, FString& OutPathFile, EFileFormat Format);
