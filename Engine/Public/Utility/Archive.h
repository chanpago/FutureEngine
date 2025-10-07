#pragma once

struct FStaticMesh;
struct FObjMaterialInfo;
enum class EFileFormat : uint8;

class FArchive
{
public:

	virtual void Serialize(void* Data, uint64 Length) = 0;
	virtual bool IsLoading() const { return false; }
	virtual bool IsSaving() const { return false; }
	virtual bool IsFileOpen() = 0;
	virtual bool IsFileClose() = 0;
	virtual bool FileOpen(const FString&) = 0;
	virtual void FileClose() = 0;
	virtual bool IsFileExist(const FString& FilePath) = 0;
	virtual bool IsBinOld(const FString& OriginalFile, const FString& BinFile, EFileFormat Format) = 0;

private:

};


#pragma region operator<<overloading

FArchive& operator<<(FArchive& Ar, int8& Value);

FArchive& operator<<(FArchive& Ar, int16& Value);

FArchive& operator<<(FArchive& Ar, int32& Value);

FArchive& operator<<(FArchive& Ar, int64& Value);

FArchive& operator<<(FArchive& Ar, uint8& Value);

FArchive& operator<<(FArchive& Ar, uint16& Value);

FArchive& operator<<(FArchive& Ar, uint32& Value);

FArchive& operator<<(FArchive& Ar, uint64& Value);

FArchive& operator<<(FArchive& Ar, float& Value);

FArchive& operator<<(FArchive& Ar, FString& Value);

FArchive& operator<<(FArchive& Ar, FVector2& Value);

FArchive& operator<<(FArchive& Ar, FVector& Value);

FArchive& operator<<(FArchive& Ar, FVector4& Value);

FArchive& operator<<(FArchive& Ar, FNormalVertex& Value);

FArchive& operator<<(FArchive& Ar, FMeshSection& Value);

template<typename T>
FArchive& operator<<(FArchive& Ar, TArray<T>& Value);

FArchive& operator<<(FArchive& Ar, FStaticMesh& Value);

FArchive& operator<<(FArchive& Ar, FObjMaterialInfo& Value);


#pragma endregion

/*
.obj 파싱 -> mtl 이름 배열 저장 -> obj .bin으로 저장 -> 배열에 존재하는 mtl만 캐싱 -> mtl .bin으로 저장
.obj 수정 -> pathfilename이 존재하지 않는데 binfileformat 존재하면 .bin 삭제 -> 둘 다 존재하고 obj가 최신 -> .obj 파싱 -> mtl 이름 배열 저장
-> obj .bin으로 저장 -> mtl .bin으로 저장
.mtl 수정 -> obj의 mtllib과 비교해서 mtl파일이 없으면 mtl .bin 수정 시작 -> 둘 다 존재 .mtl이 최신 -> .obj의 mtllib으로 새롭게 .bin 작성
.mtl과 Materials.bin과 비교
 - .mtl 삭제, Materials.bin 존재 -> Materials.bin 갱신
 - .mtl 변경 -> Materials.bin 갱신

 obj(NewStaticMesh->mtllib)에 의해 mtl이 파싱된다.
 mtl .bin 로드 성공 시 ParseMtl 함수 호출X

*/
