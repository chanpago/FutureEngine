#include "pch.h"
#include "Manager/Level/World.h"
#include "Core/EngineStatics.h"

#include "Level/Level.h"
#include "Manager/Path/PathManager.h"
#include "Manager/UI/UIManager.h"
#include "Utility/LevelSerializer.h"
#include "Utility/Metadata.h"

// TODO(Dongmin) - 스태틱 메쉬 저장을 위해 추가. 이 방법이 맞다면 추후에 순서에 맞게 수정
#include "Actor/StaticMeshActor.h"
#include "Mesh/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

#include "Editor/EditorEngine.h"
IMPLEMENT_CLASS(UWorld, UObject)

UWorld* GWorld = NewObject<UWorld>();
 
UWorld::UWorld() = default;

UWorld::UWorld(EWorldType Type)
{
	FWorldContext WorldContext;
	WorldContext.WorldType = Type;
	WorldContext.SetWorld(this);

	GEditor->AddWorldContext(WorldContext);
}

UWorld::~UWorld()
{

}

void UWorld::Init(EWorldType Type)
{
	//TODO 
	FWorldContext WorldContext;
	WorldContext.WorldType = Type;
	WorldContext.SetWorld(this);
	//UWorld* ThisCurrentWorld;

	GEditor->AddWorldContext(WorldContext);
}
 
//void UWorld::Update() const
//{
//	if (CurrentLevel)
//	{
//		CurrentLevel->Update();
//	}
//}

void UWorld::Tick()
{

	if (CurrentLevel)
	{
		//To Render
		CurrentLevel->Update();
	}
	   
	// Editor Tick  
	if (GWorld && GWorld->GetWorldType() == EWorldType::Editor)
	{
		ULevel* Level = GWorld->GetCurrentLevel();
		if (Level != nullptr)
		{
			for (AActor* Actor : Level->GetLevelActors())
			{
				if (Actor && Actor->IsTickInEditor())
				{
					Actor->Tick();
				}
			}
		}
	}

	//PIE Tick
	else if (GWorld && GWorld->GetWorldType() == EWorldType::PIE)
	{
		ULevel* Level = GWorld->GetCurrentLevel();
		{
			for (AActor* Actor : Level->GetLevelActors())
			{
				if (Actor)
				{
					Actor->Tick();
				}
			}
		} 
	}
}
/**
 * @brief New Blank Level 생성
 */
bool UWorld::CreateNewLevel(const FString& InLevelName)
{
	UE_LOG("LevelManager: Creating New Level: %s", InLevelName.c_str());
	// 이전 씬 완전 정리
	ClearAllLevels();
	// 새 레벨 생성
	ULevel* NewLevel = new ULevel(InLevelName);
	CurrentLevel = NewLevel;
	CurrentLevel->Init();

	// 기본 스냅샷 by value로 확보
	FCameraMetadata DefaultMeta = GetDefaultCameraSnapshot();
	CurrentLevel->SetSavedCameraSnapshot(DefaultMeta); // 더티만 세우고
	// 적용은 Editor 쪽에서 TryApply가 해줌
	UE_LOG("LevelManager: Successfully Created and Switched to New Level '%s'", InLevelName.c_str());

	return true;
}

/**
 * @brief 현재 레벨을 지정된 경로에 저장
 */
bool UWorld::SaveCurrentLevel(const FString& InFilePath) const
{
	if (!CurrentLevel)
	{
		UE_LOG("LevelManager: No Current Level To Save");
		return false;
	}

	// 기본 파일 경로 생성
	path FilePath = InFilePath;
	if (FilePath.empty())
	{
		// 기본 파일명은 Level 이름으로 세팅
		FilePath = GenerateLevelFilePath(CurrentLevel->GetName().empty() ? "Untitled" : CurrentLevel->GetName());
	}

	UE_LOG("LevelManager: Saving Current Level To: %s", FilePath.string().c_str());

	// LevelSerializer를 사용하여 저장
	try
	{
		// 1) 레벨에게 현재 Editor 카메라로부터 스냅샷을 받아오게
		CurrentLevel->SaveCameraSnapshotFromCamera();
		// 2) 현재 레벨의 메타데이터 생성
		FLevelMetadata Metadata = ConvertLevelToMetadata(CurrentLevel);
		// 3) 스냅샷을 메타데이터에 실어 저장
		Metadata.Camera = CurrentLevel->GetSavedCameraSnapshot();


		bool bSuccess = FLevelSerializer::SaveLevelToFile(Metadata, FilePath.string());

		if (bSuccess)
		{
			UE_LOG("LevelManager: Level Saved Successfully");
		}
		else
		{
			UE_LOG("LevelManager: Failed To Save Level");
		}

		return bSuccess;
	}
	catch (const exception& Exception)
	{
		UE_LOG("LevelManager: Exception During Save: %s", Exception.what());
		return false;
	}
}

/**
 * @brief 지정된 파일로부터 Level Load & Register
 */
bool UWorld::LoadLevel(const FString& InLevelName, const FString& InFilePath)
{
	UE_LOG("LevelManager: Loading Level '%s' From: %s", InLevelName.c_str(), InFilePath.c_str());

	// 1) 이전 씬 완전 정리(+UUID/Name 리셋)
	ClearAllLevels();

	// 2) 새 레벨 뼈대
	ULevel* NewLevel = new ULevel(InLevelName);

	try
	{
		FLevelMetadata Metadata;

		if (!FLevelSerializer::LoadLevelFromFile(Metadata, InFilePath))
		{
			UE_LOG("LevelManager: Failed To Load Level From: %s", InFilePath.c_str());
			SafeDelete(NewLevel);
			return false;
		}

		FString Error;
		if (!FLevelSerializer::ValidateLevelData(Metadata, Error))
		{
			UE_LOG("LevelManager: Level Validation Failed: %s", Error.c_str());
			SafeDelete(NewLevel);
			return false;
		}

		if (!LoadLevelFromMetadata(NewLevel, Metadata))
		{
			UE_LOG("LevelManager: Failed To Create Level From Metadata");
			SafeDelete(NewLevel);
			return false;
		}

		// 카메라 스냅샷 적용
		NewLevel->SetSavedCameraSnapshot(Metadata.Camera);
		NewLevel->ApplySavedCameraSnapshotToCamera();
	}
	catch (const std::exception& e)
	{
		UE_LOG("LevelManager: Exception During Load: %s", e.what());
		SafeDelete(NewLevel);
		return false;
	}

	CurrentLevel = NewLevel;
	CurrentLevel->Init();

	UE_LOG("LevelManager: Level '%s' loaded and activated", InLevelName.c_str());
	return true;
}
void UWorld::ClearAllLevels()
{
	FNameTable::GetInstance().Reset();      // 이름/넘버링 초기화
	// 1) 현재 레벨 포함 모든 레벨 파괴
	if (CurrentLevel)
	{
		UUIManager::GetInstance().SetSelectedActor(nullptr);
		CurrentLevel->Cleanup();
		SafeDelete(CurrentLevel);
		CurrentLevel = nullptr;
	}
	// 2) 넘버링 리셋
	
	UEngineStatics::ResetUUID();            // NextUUID = 0 등
}
void UWorld::Release()
{
	ClearAllLevels();
}


/**
 * @brief 레벨 저장 디렉토리 경로 반환
 */
UEditor* UWorld::GetOwningEditor() const
{
	return OwningEditor;
}

void UWorld::SetOwningEditor(UEditor* InEditor)
{
	OwningEditor = InEditor;
}

path UWorld::GetLevelDirectory()
{
	UPathManager& PathManager = UPathManager::GetInstance();
	return PathManager.GetWorldPath();
}

/**
 * @brief 레벨 이름을 바탕으로 전체 파일 경로 생성
 */
path UWorld::GenerateLevelFilePath(const FString& InLevelName)
{
	path LevelDirectory = GetLevelDirectory();
	path FileName = InLevelName + ".Scene";
	return LevelDirectory / FileName;

}

/**
 * @brief ULevel을 FLevelMetadata로 변환
 */
FLevelMetadata UWorld::ConvertLevelToMetadata(ULevel* InLevel)
{
	FLevelMetadata Metadata;
	Metadata.Version = 1;
	Metadata.NextUUID = 1;

	if (!InLevel)
	{
		UE_LOG("LevelManager: ConvertLevelToMetadata: Level Is Null");
		return Metadata;
	}

	// 레벨의 액터들을 순회하며 메타데이터로 변환
	uint32 CurrentID = 1;
	for (AActor* Actor : InLevel->GetLevelActors())
	{
		if (!Actor)
			continue;

		FPrimitiveMetadata PrimitiveMeta;
		PrimitiveMeta.ID = CurrentID++;
		PrimitiveMeta.Location = Actor->GetActorLocation();
		PrimitiveMeta.Rotation = Actor->GetActorRotation();
		PrimitiveMeta.Scale = Actor->GetActorScale3D();

		// TODO(Dongmin) - 우선은 임시로 구현.
		// 해결방법일지도?
		// 아마도 추후에 경로 로드는 리소스 매니저와 연관될지도
		if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::StaticMeshComp;
			// 이거 맞나..?
			PrimitiveMeta.ObjStaticMeshAsset = StaticMeshActor->GetStaticMeshComponent()->GetStaticMesh()->GetAssetPathFileName();
		}
		else
		{
			UE_LOG("LevelManager: Unknown Actor Type, Skipping...");
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}

		// 예전 방식입니다. 참고용
		// Actor 타입에 따라 EPrimitiveType 설정
		/*if (Cast<ACubeActor>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Cube;
		}
		else if (Cast<ASphereActor>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Sphere;
		}
		else if (Cast<ATriangleActor>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Triangle;
		}
		else if (Cast<ASquareActor>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Square;
		}
		else
		{
			UE_LOG("LevelManager: Unknown Actor Type, Skipping...");
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}*/

		Metadata.Primitives[PrimitiveMeta.ID] = PrimitiveMeta;
	}

	Metadata.NextUUID = CurrentID;

	UE_LOG("LevelManager: Converted %zu Actors To Metadata", Metadata.Primitives.size());
	// 카메라 스냅샷도 같이 채움
	// (Save에서 이미 SaveCameraSnapshotFromCamera 호출했지만, 안전하게 한 번 더 동기화)
	Metadata.Camera = InLevel->GetSavedCameraSnapshot();

	return Metadata;
}

/**
 * @brief FLevelMetadata로부터 ULevel에 Actor Load
 */
bool UWorld::LoadLevelFromMetadata(ULevel* InLevel, const FLevelMetadata& InMetadata)
{
	if (!InLevel)
	{
		UE_LOG("LevelManager: LoadLevelFromMetadata: InLevel Is Null");
		return false;
	}

	UE_LOG("LevelManager: Loading %zu Primitives From Metadata", InMetadata.Primitives.size());
	FAABB OctreeSize;
	TArray<UPrimitiveComponent*> ComponentList;
	ComponentList.reserve(InMetadata.Primitives.Num());

	// Metadata의 각 Primitive를 Actor로 생성
	for (const auto& [ID, PrimitiveMeta] : InMetadata.Primitives)
	{
		AActor* NewActor = nullptr;
		UResourceManager& ResourceManager = UResourceManager::GetInstance();
		UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh(PrimitiveMeta.ObjStaticMeshAsset);


		// TODO(Dongmin) - 우선은 임시로 구현.
		// 해결방법일지도?
		// 아마 이곳에서 리소스 매니저에게 넘겨서 파일의 경로를 로드하게 해야 할 수도 있다.
		switch (PrimitiveMeta.Type)
		{
		case EPrimitiveType::StaticMeshComp:
			if (StaticMesh)
			{
				NewActor = InLevel->SpawnActor<AStaticMeshActor>();
				NewActor->SetStaticMesh(StaticMesh);
			}
			break;
		default:
			UE_LOG("LevelManager: Unknown Primitive Type: %d", static_cast<int32>(PrimitiveMeta.Type));
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}

		if (NewActor)
		{
			// Transform 정보 적용
			NewActor->SetActorLocation(PrimitiveMeta.Location);
			NewActor->SetActorRotation(PrimitiveMeta.Rotation);
			NewActor->SetActorScale3D(PrimitiveMeta.Scale);

			UE_LOG("LevelManager: (%.2f, %.2f, %.2f) 지점에 %s (을)를 생성했습니다 ",
			       PrimitiveMeta.Location.X, PrimitiveMeta.Location.Y, PrimitiveMeta.Location.Z,
			       FLevelSerializer::PrimitiveTypeToWideString(PrimitiveMeta.Type).c_str());
			UPrimitiveComponent* Component = static_cast<UPrimitiveComponent*>(NewActor->GetRootComponent());
			ComponentList.Add(Component);
			OctreeSize.AddAABB(Component->GetWorldBounds());
		}
		else
		{
			UE_LOG("LevelManager: Actor 생성에 실패했습니다 (Primitive ID: %d)", ID);
		}
	}

	InLevel->NewOctree(OctreeSize);
	for (UPrimitiveComponent* Component : ComponentList)
	{
		InLevel->AddToOctree(Component);
	}
	
	UE_LOG("LevelManager: 레벨이 메타데이터로부터 성공적으로 로드되었습니다");
	return true;
}

UWorld* UWorld::DuplicateWorldForPIE(UWorld* EditorWorld)
{
	//TODO 싱글톤해제 시켜야 됨 
	UWorld* Dst = NewObject<UWorld>();
	Dst->Init(EWorldType::PIE);

	Dst->SetWorldType(EWorldType::PIE);

	ULevel* SrcLevel = EditorWorld->GetCurrentLevel();
	ULevel* DstLevel = new ULevel(SrcLevel->GetName() + "_PIE");
	
	Dst->SetCurrentLevel(DstLevel);

	for (AActor* Actor : SrcLevel->GetLevelActors())
	{
		//Actor->DuplicateForTest(Dst->GetCurrentLevel() , Actor); 
		AActor* DupActor = static_cast<AActor*>(Actor->Duplicate());  
		if (!DupActor) continue;
		// Rebind duplicate to PIE level
		DstLevel->AddActor(DupActor);
	}

	return Dst;
}

void UWorld::InitializeActorsForPlay()
{
	ULevel* DupLevel = GetCurrentLevel();
	if (!DupLevel) return;

	//Actor 초기화
	for (AActor* Actor : DupLevel->GetLevelActors())
	{
		if (Actor == nullptr) continue;

		Actor->BeginPlay();

	}
}

void UWorld::CleanUpWorld()
{ 
	// 깊은 복사 부분만 다 날리기 
}

