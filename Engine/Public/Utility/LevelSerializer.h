#pragma once

namespace json {
    class JSON;
}

using JSON = json::JSON;

enum class EPrimitiveType : uint8_t;
struct FLevelMetadata;
struct FPrimitiveMetadata;
struct FCameraMetadata;
/**
 * @brief Level 직렬화에 관여하는 클래스
 * JSON 기반으로 레벨의 데이터를 Save / Load 처리
 */
class FLevelSerializer
{
public:
	static JSON VectorToJson(const FVector& InVector);
	static FVector JsonToVector(const JSON& InJsonData);
	static string PrimitiveTypeToWideString(EPrimitiveType InType);
	static EPrimitiveType StringToPrimitiveType(const string& InTypeString);
	static JSON PrimitiveMetadataToJson(const FPrimitiveMetadata& InPrimitive);
	static FPrimitiveMetadata JsonToPrimitive(const JSON& InJsonData, uint32 InID);
	static JSON LevelToJson(const FLevelMetadata& InLevelData);
	static FLevelMetadata JsonToLevel(JSON& InJsonData);
	// 카메라 정보 저장/로드
	static JSON CameraToJson(const FCameraMetadata& InCamera);
	static FCameraMetadata JsonToCamera(const JSON& InJson);

	static bool SaveLevelToFile(const FLevelMetadata& InLevelData, const string& InFilePath);
	static bool LoadLevelFromFile(FLevelMetadata& OutLevelData, const string& InFilePath);
	static string FormatJsonString(const JSON& JsonData, int Indent = 2);
	static bool ValidateLevelData(const FLevelMetadata& InLevelData, string& OutErrorMessage);
	static FLevelMetadata MergeLevelData(const FLevelMetadata& InBaseLevel,
	                                     const FLevelMetadata& InMergeLevel);


	static TArray<FPrimitiveMetadata>
	FilterPrimitivesByType(const FLevelMetadata& InLevelData, EPrimitiveType InType);

	struct FLevelStats
	{
		uint32 TotalPrimitives = 0;
		TMap<EPrimitiveType, uint32> PrimitiveCountByType;
		// FVector BoundingBoxMin;
		// FVector BoundingBoxMax;
	};

	static FLevelStats GenerateLevelStats(const FLevelMetadata& InLevelData);

private:
	static bool HandleJsonError(const exception& InException, const string& InContext,
	                            string& OutErrorMessage);
};
