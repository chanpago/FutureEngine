#include "pch.h"
#include "Utility/LevelSerializer.h"

#include "json.hpp"
#include "Utility/Metadata.h"

using json::JSON;


// 타입 문자열 얻기 (enum → 문자열). 커스텀 필드가 있으면 그걸 먼저 사용.
static std::string GetPrimitiveTypeString(const FPrimitiveMetadata& P)
{
	// 예) 커스텀 이름이 있다면 우선
	// if (!P.TypeName.empty()) return P.TypeName;

	return FLevelSerializer::PrimitiveTypeToWideString(P.Type);
}
// ===== Helpers: trim / strip / collapse / indent(no blank) =====
static std::string LTrim(const std::string& S) {
	size_t I = 0; while (I < S.size() && std::isspace((unsigned char)S[I])) ++I;
	return S.substr(I);
}
static std::string RTrim(const std::string& S) {
	size_t I = S.size(); while (I > 0 && std::isspace((unsigned char)S[I - 1])) --I;
	return S.substr(0, I);
}
// 양옆 공백 제거
static std::string Trim(const std::string& S) { return RTrim(LTrim(S)); }

// ===== numeric & vector format helpers =====
static std::string FormatFloat(float V, int Precision)
{
	std::ostringstream Os;
	Os.setf(std::ios::fixed);
	Os << std::setprecision(Precision) << V;
	return Os.str();
}

// 고정 소수점으로 Precision 자리수까지 숫자를 문자열로 바꿔주는 헬퍼
static std::string FormatVec3(const FVector& V, int Precision = 6)
{
	return "[ " + FormatFloat(V.X, Precision) + ", "
		+ FormatFloat(V.Y, Precision) + ", "
		+ FormatFloat(V.Z, Precision) + " ]";
}

// "  " * BaseIndent 만큼 왼쪽 들여쓰기
static std::string Indent(int BaseIndent) { return std::string(BaseIndent * 2, ' '); }

// 하나의 Primitive KV를 조립:  "  \"3\": { ... }"
static std::string FormatPrimitiveKeyValue(uint32 Id, const FPrimitiveMetadata& P, int BaseIndent = 2)
{
	const std::string I0 = Indent(BaseIndent);       //   "  "
	const std::string I1 = Indent(BaseIndent + 1);   //   "    "

	std::ostringstream Os;
	Os << I0 << "\"" << Id << "\": {\n";
	// ★ StaticMeshComp이면 Asset 경로를 먼저 출력 (스키마 고정)
	if (P.Type == EPrimitiveType::StaticMeshComp)
	{
		// 비어 있어도 스키마 일관성 위해 항상 출력
		Os << I1 << "\"ObjStaticMeshAsset\": " << std::quoted(P.ObjStaticMeshAsset) << ",\n";
	}
	Os << I1 << "\"Location\": " << FormatVec3(P.Location, 6) << ",\n";
	Os << I1 << "\"Rotation\": " << FormatVec3(P.Rotation, 6) << ",\n";
	Os << I1 << "\"Scale\": " << FormatVec3(P.Scale, 6) << ",\n";
	Os << I1 << "\"Type\": " << "\"" << GetPrimitiveTypeString(P) << "\"\n";
	Os << I0 << "}";
	return Os.str();
}

/**
 * @brief FVector를 JSON으로 변환
 */
JSON FLevelSerializer::VectorToJson(const FVector& InVector)
{
	JSON VectorArray = JSON::Make(JSON::Class::Array);
	VectorArray.append(InVector.X, InVector.Y, InVector.Z);
	return VectorArray;
}

/**
 * @brief JSON을 FVector로 변환
 * 적절하지 않은 JSON을 입력 받았을 경우 혹은 파싱에서 오류가 발생한 경우에 Zero Vector를 반환한다
 * 따로 로드에서 발생하는 에러는 일단 핸들링하지 않음
 */
FVector FLevelSerializer::JsonToVector(const JSON& InJsonData)
{
	if (InJsonData.JSONType() != JSON::Class::Array || InJsonData.size() != 3)
	{
		return { static_cast<float>(InJsonData.at(0).ToFloat()), 0.0f, 0.0f };
	}

	try
	{
		return {
			static_cast<float>(InJsonData.at(0).ToFloat()),
			static_cast<float>(InJsonData.at(1).ToFloat()),
			static_cast<float>(InJsonData.at(2).ToFloat())
		};
	}
	catch (const exception&)
	{
		return {0.0f, 0.0f, 0.0f};
	}
}

/**
 * @brief EPrimitiveType을 문자열로 변환
 */
FString FLevelSerializer::PrimitiveTypeToWideString(EPrimitiveType InType)
{
	switch (InType)
	{
	case EPrimitiveType::Sphere:
		return "Sphere";
	case EPrimitiveType::Cube:
		return "Cube";
	case EPrimitiveType::Triangle:
		return "Triangle";
	case EPrimitiveType::Square:
		return "Square";
	case EPrimitiveType::StaticMeshComp:
		return "StaticMeshComp";
	default:
		return "Unknown";
	}
}

/**
 * @brief 문자열을 EPrimitiveType으로 변환
 */
EPrimitiveType FLevelSerializer::StringToPrimitiveType(const FString& InTypeString)
{
	if (InTypeString == "Sphere")
	{
		return EPrimitiveType::Sphere;
	}
	if (InTypeString == "Cube")
	{
		return EPrimitiveType::Cube;
	}
	if (InTypeString == "Triangle")
	{
		return EPrimitiveType::Triangle;
	}
	if (InTypeString == "Square")
	{
		return EPrimitiveType::Square;
	}
	if (InTypeString == "StaticMeshComp")
	{
		return EPrimitiveType::StaticMeshComp;
	}

	return EPrimitiveType::None;
}

/**
 * @brief FPrimitiveMetadata를 JSON으로 변환
 */
JSON FLevelSerializer::PrimitiveMetadataToJson(const FPrimitiveMetadata& InPrimitive)
{
	JSON PrimitiveJson;
	// Maybe TODO(Dongmin) - Ex) "ObjStaticMeshAsset": "Data/Cube.obj"
	PrimitiveJson["ObjStaticMeshAsset"] = InPrimitive.ObjStaticMeshAsset;
	PrimitiveJson["Location"] = VectorToJson(InPrimitive.Location);
	PrimitiveJson["Rotation"] = VectorToJson(InPrimitive.Rotation);
	PrimitiveJson["Scale"] = VectorToJson(InPrimitive.Scale);
	PrimitiveJson["Type"] = PrimitiveTypeToWideString(InPrimitive.Type);
	return PrimitiveJson;
}

/**
 * @brief JSON을 FPrimitiveData로 변환
 */
FPrimitiveMetadata FLevelSerializer::JsonToPrimitive(const JSON& InJsonData, uint32 InID)
{
	FPrimitiveMetadata PrimitiveMeta;
	PrimitiveMeta.ID = InID;

	try
	{
		if (InJsonData.JSONType() == JSON::Class::Object)
		{
			// TODO(Dongmin) - ObjStaticMeshAsset 정보도 저장하기
			const JSON& ObjStaticMeshAssetJson = InJsonData.at("ObjStaticMeshAsset");
			const JSON& LocationJson = InJsonData.at("Location");
			const JSON& RotationJson = InJsonData.at("Rotation");
			const JSON& ScaleJson = InJsonData.at("Scale");
			const JSON& TypeJson = InJsonData.at("Type");
			PrimitiveMeta.ObjStaticMeshAsset = ObjStaticMeshAssetJson.ToString();
			PrimitiveMeta.Location = JsonToVector(LocationJson);
			PrimitiveMeta.Rotation = JsonToVector(RotationJson);
			PrimitiveMeta.Scale = JsonToVector(ScaleJson);
			PrimitiveMeta.Type = StringToPrimitiveType(TypeJson.ToString());

			UE_LOG("LevelSerializer: JsonToPrimitive: ID: %d | Scale: (%.3f, %.3f, %.3f)",
			       InID, PrimitiveMeta.Scale.X, PrimitiveMeta.Scale.Y, PrimitiveMeta.Scale.Z);
		}
	}
	catch (const exception&)
	{
		// JSON 파싱 실패 시 기본값 유지
	}

	return PrimitiveMeta;
}

/**
 * @brief FLevelMetadata를 JSON으로 변환
 */
JSON FLevelSerializer::LevelToJson(const FLevelMetadata& InLevelData)
{
	JSON LevelJson;
	LevelJson["Version"] = InLevelData.Version;
	LevelJson["NextUUID"] = InLevelData.NextUUID;

	JSON PrimitivesJson;
	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		PrimitivesJson[to_string(ID)] = PrimitiveMetadataToJson(Primitive);
	}
	LevelJson["Primitives"] = PrimitivesJson;

	// ▼ 카메라 스냅샷 추가
	LevelJson["PerspectiveCamera"] = CameraToJson(InLevelData.Camera);

	return LevelJson;
}

/**
 * @brief JSON을 FLevelData로 변환
 */
FLevelMetadata FLevelSerializer::JsonToLevel(JSON& InJsonData)
{
	FLevelMetadata LevelData;

	try
	{
		// 버전 정보 파싱
		if (InJsonData.hasKey("Version"))
		{
			LevelData.Version = InJsonData.at("Version").ToInt();
		}

		// NextUUID 파싱
		if (InJsonData.hasKey("NextUUID"))
		{
			LevelData.NextUUID = InJsonData["NextUUID"].ToInt();
		}

		// Primitives 파싱
		if (InJsonData.hasKey("Primitives") &&
			InJsonData["Primitives"].JSONType() == JSON::Class::Object)
		{
			const auto& PrimitivesJson = InJsonData["Primitives"];
			for (const auto& InPair : PrimitivesJson.ObjectRange())
			{
				try
				{
					uint32 ID = stoul(InPair.first);
					FPrimitiveMetadata Primitive = JsonToPrimitive(InPair.second, ID);
					LevelData.Primitives[ID] = Primitive;
				}
				catch (const exception&)
				{
					cout << "[JSON PARSER] Failed To Load Primitive From JSON" << "\n";
					continue;
				}
			}
		}
		// ▼ 카메라
		if (InJsonData.hasKey("PerspectiveCamera"))
		{
			LevelData.Camera = JsonToCamera(InJsonData.at("PerspectiveCamera"));
		}
	}
	catch (const exception&)
	{
		cout << "[JSON PARSER] Failed To Load Level From JSON" << "\n";
	}

	return LevelData;
}
/** @brief FCameraMetadata -> JSON ("PerspectiveCamera") */
JSON FLevelSerializer::CameraToJson(const FCameraMetadata& InCamera)
{
	JSON Cam;
	Cam["Location"] = VectorToJson(InCamera.Location);
	Cam["Rotation"] = VectorToJson(InCamera.Rotation);
	Cam["FOV"] = VectorToJson(FVector(InCamera.Fov,0,0));
	Cam["NearClip"] = VectorToJson(FVector(InCamera.NearZ,0,0));
	Cam["FarClip"] = VectorToJson(FVector(InCamera.FarZ,0,0));
	return Cam;
}

/** @brief JSON -> FCameraMetadata */
FCameraMetadata FLevelSerializer::JsonToCamera(const JSON& InJson)
{
	FCameraMetadata Cam;
	try
	{
		if (InJson.JSONType() == JSON::Class::Object)
		{
			if (InJson.hasKey("Location")) Cam.Location = JsonToVector(InJson.at("Location"));
			if (InJson.hasKey("Rotation")) Cam.Rotation = JsonToVector(InJson.at("Rotation"));

			if (InJson.hasKey("FOV"))      Cam.Fov = JsonToVector(InJson.at("FOV")).X;
			if (InJson.hasKey("NearClip")) Cam.NearZ = JsonToVector(InJson.at("NearClip")).X;
			if (InJson.hasKey("FarClip"))  Cam.FarZ = JsonToVector(InJson.at("FarClip")).X;
		}
	}
	catch (const exception&)
	{
		// 파싱 실패 시 기본값 유지
	}
	return Cam;
}
/**
 * @brief 레벨 데이터를 파일에 저장
 */
bool FLevelSerializer::SaveLevelToFile(const FLevelMetadata& InLevelData, const FString& InFilePath)
{
	try {
		// 0) 고정 값
		const std::string VersionStr = std::to_string(InLevelData.Version);

		// 1) 원본 ID 수집 → 정렬
		std::vector<uint32> OrigIds;
		OrigIds.reserve(InLevelData.Primitives.size());
		for (const auto& KeyValue : InLevelData.Primitives) OrigIds.push_back(KeyValue.first);
		std::sort(OrigIds.begin(), OrigIds.end());

		// 2) NextUUID = N + 1
		const uint32 UsedMaxId = OrigIds.empty() ? 0u : OrigIds.back();
		const uint32 NewNextUUID = std::max(InLevelData.NextUUID, UsedMaxId + 1);
		const std::string NextUuidStr = std::to_string(NewNextUUID);

		// 3) Primitives: 1..N 리넘버해서 수동 조립 (빈 줄 없음, 순서 Location→Rotation→Scale→Type)
		std::ostringstream PrimsOs;
		PrimsOs << "  \"Primitives\": {\n";
		for (size_t i = 0; i < OrigIds.size(); ++i)
		{
			const uint32 oldId = OrigIds[i];
			FPrimitiveMetadata P = InLevelData.Primitives.at(oldId); // 복사본
			const uint32 newId = static_cast<uint32>(i + 1);
			P.ID = newId;

			PrimsOs << FormatPrimitiveKeyValue(newId, P, /*BaseIndent=*/2);
			if (i + 1 < OrigIds.size()) PrimsOs << ",\n";
			else                        PrimsOs << "\n";
		}
		PrimsOs << "  }";

		// 4) Camera: 순서 고정(Location→Rotation→FOV→NearClip→FarClip)
		const FCameraMetadata& C = InLevelData.Camera;
		std::ostringstream CamOs;
		CamOs << "  \"PerspectiveCamera\": {\n"
			<< "    \"Location\": " << FormatVec3(C.Location, 6) << ",\n"
			<< "    \"Rotation\": " << FormatVec3(C.Rotation, 6) << ",\n"
			<< "    \"FOV\": " << FormatVec3(FVector(C.Fov,0,0), 1) << ",\n"
			<< "    \"NearClip\": " << FormatVec3(FVector(C.NearZ,0,0), 6) << ",\n"
			<< "    \"FarClip\": " << FormatVec3(FVector(C.FarZ,0,0), 6) << "\n"
			<< "  }";

		// 5) 최종 조립
		std::ostringstream Os;
		Os << "{\n";
		Os << "  \"Version\": " << VersionStr << ",\n";
		Os << "  \"NextUUID\": " << NextUuidStr << ",\n";
		Os << PrimsOs.str() << ",\n";
		Os << CamOs.str() << "\n";
		Os << "}\n";

		std::ofstream File(InFilePath);
		if (!File.is_open()) return false;
		File << Os.str();
		File.close();
		return true;
	}
	catch (...) {
		return false;
	}
}

/**
 * @brief 파일에서 레벨 데이터 로드
 */
bool FLevelSerializer::LoadLevelFromFile(FLevelMetadata& OutLevelData, const FString& InFilePath)
{
	try
	{
		ifstream File(InFilePath);
		if (!File.is_open())
		{
			return false;
		}

		// 파일 전체 내용을 한 번에 읽도록 처리
		File.seekg(0, std::ios::end);
		size_t FileSize = File.tellg();

		File.seekg(0, std::ios::beg);

		FString FileContent;
		FileContent.resize(FileSize);
		File.read(FileContent.data(), FileSize);
		File.close();

		cout << "[LevelSerializer] File Content Length: " << FileContent.length() << "\n";

		JSON JsonData = JSON::Load(FileContent);
		OutLevelData = JsonToLevel(JsonData);

		return true;
	}
	catch (const exception&)
	{
		return false;
	}
}

/**
 * @brief JSON 문자열을 예쁘게 포맷팅
 * 지금은 사용하지 않는다.
 */
FString FLevelSerializer::FormatJsonString(const JSON& JsonData, int Indent)
{
	return JsonData.dump(Indent);
}

/**
 * @brief 레벨 데이터 유효성 검사
 */
bool FLevelSerializer::ValidateLevelData(const FLevelMetadata& InLevelData, FString& OutErrorMessage)
{
	OutErrorMessage.clear();

	// 버전 체크
	if (InLevelData.Version == 0)
	{
		OutErrorMessage = "Invalid Version: Version Must Be Greater Than 0";
		return false;
	}

	// NextUUID 체크
	uint32 MaxUsedID = 0;
	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		MaxUsedID = max(MaxUsedID, ID);

		// ID 일관성 체크
		if (Primitive.ID != ID)
		{
			OutErrorMessage = "ID Mismatch: Primitive ID (" + to_string(Primitive.ID) +
				") Doesn't Match Map Key (" + to_string(ID) + ")";
			return false;
		}

		// 타입 체크
		if (Primitive.Type == EPrimitiveType::None)
		{
			OutErrorMessage = "Invalid Primitive Type For ID " + to_string(ID);
			return false;
		}

		// 스케일 체크 (0이면 안됨)
		if (Primitive.Scale.X == 0.0f || Primitive.Scale.Y == 0.0f || Primitive.Scale.Z == 0.0f)
		{
			OutErrorMessage = "Invalid Scale For Primitive ID " + to_string(ID) +
				": Scale Components Must Be Non-Zero";
			return false;
		}
	}

	// NextUUID가 사용된 ID보다 큰지 체크
	if (!InLevelData.Primitives.empty() && InLevelData.NextUUID <= MaxUsedID)
	{
		OutErrorMessage = "Invalid NextUUID: Must Be Greater Than The Highest Used ID (" +
			to_string(MaxUsedID) + ")";
		return false;
	}

	// 카메라 유효성
	if (InLevelData.Camera.Fov <= 0.0f)
	{
		OutErrorMessage = "Invalid Camera: FOV must be > 0";
		return false;
	}
	if (InLevelData.Camera.FarZ <= InLevelData.Camera.NearZ)
	{
		OutErrorMessage = "Invalid Camera: FarClip must be greater than NearClip";
		return false;
	}

	return true;
}

/**
 * @brief 두 레벨 데이터 병합 (중복 ID는 덮어 씀)
 * XXX(KHJ): 현재는 필요하지 않은 상황인데 필요한 경우가 존재할지 고민 후 제거해도 될 듯
 */
FLevelMetadata FLevelSerializer::MergeLevelData(const FLevelMetadata& InBaseLevel,
                                                const FLevelMetadata& InMergeLevel)
{
	FLevelMetadata ResultLevel = InBaseLevel;

	// 버전은 더 높은 것으로
	ResultLevel.Version = max(InBaseLevel.Version, InMergeLevel.Version);

	// MergeLevel의 프리미티브들을 추가 / 덮어쓰기
	for (const auto& [ID, Primitive] : InMergeLevel.Primitives)
	{
		ResultLevel.Primitives[ID] = Primitive;
	}

	// NextUUID 업데이트
	ResultLevel.NextUUID = max(InBaseLevel.NextUUID, InMergeLevel.NextUUID);

	return ResultLevel;
}

/**
 * @brief 특정 타입의 프리미티브들만 필터링
 */
TArray<FPrimitiveMetadata> FLevelSerializer::FilterPrimitivesByType(const FLevelMetadata& InLevelData,
                                                                    EPrimitiveType InType)
{
	TArray<FPrimitiveMetadata> FilteredPrimitives;

	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		if (Primitive.Type == InType)
		{
			FilteredPrimitives.push_back(Primitive);
		}
	}

	return FilteredPrimitives;
}

/**
 * @brief 레벨 데이터 통계 정보 생성
 */
FLevelSerializer::FLevelStats FLevelSerializer::GenerateLevelStats(const FLevelMetadata& InLevelData)
{
	FLevelStats Stats;

	if (InLevelData.Primitives.empty())
	{
		return Stats;
	}

	Stats.TotalPrimitives = static_cast<uint32>(InLevelData.Primitives.size());

	// 바운딩 박스 초기화
	// bool bFirstPrimitive = true;

	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		// 타입별 카운트
		Stats.PrimitiveCountByType[Primitive.Type]++;

		// 바운딩 박스 계산
		// if (bFirstPrimitive)
		// {
		// 	Stats.BoundingBoxMin = Primitive.Location;
		// 	Stats.BoundingBoxMax = Primitive.Location;
		// 	bFirstPrimitive = false;
		// }
		// else
		// {
		// 	Stats.BoundingBoxMin.X = min(Stats.BoundingBoxMin.X, Primitive.Location.X);
		// 	Stats.BoundingBoxMin.Y = min(Stats.BoundingBoxMin.Y, Primitive.Location.Y);
		// 	Stats.BoundingBoxMin.Z = min(Stats.BoundingBoxMin.Z, Primitive.Location.Z);
		//
		// 	Stats.BoundingBoxMax.X = max(Stats.BoundingBoxMax.X, Primitive.Location.X);
		// 	Stats.BoundingBoxMax.Y = max(Stats.BoundingBoxMax.Y, Primitive.Location.Y);
		// 	Stats.BoundingBoxMax.Z = max(Stats.BoundingBoxMax.Z, Primitive.Location.Z);
		// }
	}

	return Stats;
}

