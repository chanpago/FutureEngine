#pragma once

struct FNormalVertex;
class UStaticMesh;
class FMtlParser;
class UMaterial;
enum class EFileFormat : uint8;

class FObjManager
{
	DECLARE_SINGLETON(FObjManager);

public:
	void LoadPresetMaterial();

	UStaticMesh* LoadObjStaticMesh(const FString& PathFileName);

	static UMaterial* LoadMaterial(const FString& FilePath) { if (Materials.Find(FilePath)) return Materials[FilePath]; else return Materials["None"]; }
private:
	static FStaticMesh* LoadObjStaticMeshAsset(const FString& PathFileName);
	static FStaticMesh* LoadObj(const FString& FilePath);
	static bool ParseFaceTriplet(const FString& s, int32& v, int32& vt, int32& vn);
	static bool ParseObjRaw(const FString& FilePath, FObjInfo& OutRawData);
	static bool CookObjToStaticMesh(const FObjInfo& Raw, const FObjImportOption& Opt, FStaticMesh& OutMesh);
	void ReleaseStaticMesh();
	void ReleaseMtlInfo();
	static void SaveToObjBinFile(const FString& PathFileName, FStaticMesh& NewMesh, EFileFormat Format);
	static FStaticMesh* LoadFromObjBinFile(const FString& PathFileName);
	

private:
	static TMap<FString, FStaticMesh*> ObjStaticMap;

	// TMap<mtl파일명, TMap<mtl파일 안의 mtl명, mtl정보포인터>>
	static TMap<FString, UMaterial*> Materials;

	static FMtlParser* MtlManager;

	static bool bIsObjParsing;
};



static inline FVector PositionToUEBasis(const FVector& InVector) { return FVector(InVector.X, -InVector.Y, InVector.Z); }

static inline FVector2 UVToBasis(const FVector2& InVector) { return FVector2(InVector.X, 1.0f - InVector.Y); }

// 인덱스가 음수인 경우 마지막 위치에서의 상대적 위치를 의미한다.
// 10개의 정점이 존재할 때 인덱스가 -3이면 10 - 1 = 7번 째 인덱스
static inline int32 ResolveIndex(int32 i, int32 count) { return (i > 0) ? (i - 1) : (count + i); }
