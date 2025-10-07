#include "pch.h"
#include "Render/UI/Widget/PrimitiveSpawnWidget.h"

#include "Level/Level.h"
#include "Manager/Level/World.h"
#include "Actor/StaticMeshActor.h"

IMPLEMENT_CLASS(UPrimitiveSpawnWidget, UWidget)

UPrimitiveSpawnWidget::UPrimitiveSpawnWidget()
{
}

UPrimitiveSpawnWidget::~UPrimitiveSpawnWidget() = default;

void UPrimitiveSpawnWidget::Initialize()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::Update()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::RenderWidget()
{
	ImGui::Text("Primitive Actor 생성");
	ImGui::Spacing();

	// Primitive 타입 선택 DropDown
	const char* PrimitiveTypes[] = {
		"Cube",
		"Sphere",
		"Triangle",
		"Square",
		"Minion",
		"Trees"
	};

	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::Combo("##PrimitiveType", &SelectedPrimitiveType, PrimitiveTypes, 6);

	// Spawn 버튼과 개수 입력
	ImGui::Text("Number of Spawn:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##NumberOfSpawn", &NumberOfSpawn);
	NumberOfSpawn = max(1, NumberOfSpawn);
	NumberOfSpawn = min(10000, NumberOfSpawn);

	ImGui::SameLine();
	if (ImGui::Button("Spawn Actors"))
	{
		SpawnActors();
	}

	// 스폰 범위 설정
	ImGui::Text("Spawn Range:");
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Min##SpawnRange", &SpawnRangeMin, 0.1f, -50.0f, SpawnRangeMax - 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Max##SpawnRange", &SpawnRangeMax, 0.1f, SpawnRangeMin + 0.1f, 50.0f);

	ImGui::Separator();
}

/**
 * @brief Actor 생성 함수
 * 난수를 활용한 Range, Size, Rotion 값 생성으로 Actor Spawn
 */
void UPrimitiveSpawnWidget::SpawnActors() const
{ 
	ULevel* CurrentLevel = GWorld->GetCurrentLevel();
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	if (!CurrentLevel)
	{
		UE_LOG("ControlPanel: No Current Level To Spawn Actors");
		return;
	}

	AActor* NewActor = nullptr;
	// 지정된 개수만큼 액터 생성
	for (int32 i = 0; i < NumberOfSpawn; i++)
	{
		if (SelectedPrimitiveType == 0)
		{
			UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh("Data/cube-tex.obj");
			if(StaticMesh)
			{
				NewActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
				NewActor->SetStaticMesh(StaticMesh); ;
			}
			else
			{
				break;
			}
		}
		else if (SelectedPrimitiveType == 1)
		{
			UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh("Data/sphere.obj");
			if (StaticMesh)
			{
				NewActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
				NewActor->SetStaticMesh(StaticMesh);
			}
			else
			{
				break;
			}
		}
		else if (SelectedPrimitiveType == 2)
		{
			UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh("Data/triangle.obj");
			if (StaticMesh)
			{
				NewActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
				NewActor->SetStaticMesh(StaticMesh);
			}
			else
			{
				break;
			}
		}
		else if (SelectedPrimitiveType == 3)
		{
			UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh("Data/square.obj");
			if (StaticMesh)
			{
				NewActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
				NewActor->SetStaticMesh(StaticMesh);
			}
			else
			{
				break;
			}
		}
		else if (SelectedPrimitiveType == 4)
		{
			UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh("Data/minion.obj");
			if (StaticMesh)
			{
				NewActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
				NewActor->SetStaticMesh(StaticMesh);
			}
			else
			{
				break;
			}
		}
		else if (SelectedPrimitiveType == 5)
		{
			UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh("Data/trees9.obj");
			if (StaticMesh)
			{
				NewActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
				NewActor->SetStaticMesh(StaticMesh);
			}
			else
			{
				break;
			}
		}

		if (NewActor)
		{
			// 범위 내 랜덤 위치
			float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);

			NewActor->SetActorLocation(FVector(RandomX, RandomY, RandomZ));
			NewActor->SetInitPos(FVector(RandomX, RandomY, RandomZ));

			// 임의의 스케일 (0.5 ~ 2.0 범위)
			float RandomScale = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
			NewActor->SetActorScale3D(FVector(RandomScale, RandomScale, RandomScale));

			//CurrentLevel->AddToOctree(Cast<UPrimitiveComponent>(NewActor->GetRootComponent()));

			UE_LOG("ControlPanel: (%.2f, %.2f, %.2f) 지점에 Actor를 생성했습니다", RandomX, RandomY, RandomZ);
		}
	}
	if(!NewActor)
	{
		UE_LOG("ControlPanel: Actor 생성에 실패했습니다");
	}
	else
	{
		UE_LOG("ControlPanel: %s 타입의 Actor를 %d개 생성했습니다",
			(SelectedPrimitiveType == 0 ? "Cube" : "Sphere"), NumberOfSpawn);
	}
}
