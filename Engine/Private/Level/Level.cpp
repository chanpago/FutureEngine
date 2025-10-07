#include "pch.h"
#include "Level/Level.h"

#include "Actor/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "Render/Renderer/Renderer.h"
#include "Utility/Metadata.h"
#include "Manager/UI/UIManager.h"
IMPLEMENT_CLASS(ULevel, UObject)

ULevel::ULevel() = default;

ULevel::ULevel(const FString& InName)
	: UObject(InName)
{
}

ULevel::~ULevel()
{
	Cleanup();
}

// 원래는 레벨 내부 리소스 생성하는 것이 이상적으로 보이지만, 지금은 테스트 코드만 넣어둠
void ULevel::Init()
{
	// TEST CODE
}

void ULevel::Update()
{
	// Process Delayed Task
	ProcessPendingDeletions();

	uint64 AllocatedByte = GetAllocatedBytes();
	uint32 AllocatedCount = GetAllocatedCount();
	StaticOctree.Rearrange();

	TextComponentsToRender.clear();
	BillboardComponentsToRender.clear();
	StaticMeshComponentsToRender.clear();

	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			UpdateComponentsToRender(Actor);
		}
	} 
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
	UUIManager::GetInstance().SetSelectedActor(nullptr);
	// 지연 삭제 먼저
	for (AActor*& A : ActorsToDelete)
	{
		A->SetLevel(nullptr);         // 역참조 해제
		RemoveFromRenderQueues(A);
		SafeDelete(A);
	}
	ActorsToDelete.clear();

	// 모든 액터 삭제
	for (AActor*& A : LevelActors)
	{
		A->SetLevel(nullptr);         // 역참조 해제
		RemoveFromRenderQueues(A);
		SafeDelete(A);
	}
	LevelActors.clear();
	TextComponentsToRender.clear();
	StaticOctree.clear();
}

void ULevel::UpdateComponentsToRender(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }
    for (auto& Component : Actor->GetOwnedComponents())
    {
        
        if (Component->IsA(UPrimitiveComponent::StaticClass()))
        {

            UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
			if (!PrimitiveComponent->IsVisible())
			{
				continue;
			}
            if ( PrimitiveComponent->IsMoved())
            {
				StaticOctree.UpdateElement(PrimitiveComponent);
				PrimitiveComponent->NotMove();
            }
			if (Component->IsA(UTextRenderComponent::StaticClass()))
			{
				UTextRenderComponent* TextComponent = static_cast<UTextRenderComponent*>(Component);
				
				TextComponentsToRender.Add(TextComponent);
			}
			if (Component->IsA(UStaticMeshComponent::StaticClass()))
			{
				StaticMeshComponentsToRender.Add(static_cast<UStaticMeshComponent*>(Component));
			}
			// Billboard
			if (Component->IsA(UBillboardComponent::StaticClass()))
			{
				BillboardComponentsToRender.Add(static_cast<UBillboardComponent*>(Component));
			}
        }
		
    }
}

void ULevel::AddToOctree(UPrimitiveComponent* Component)
{
    StaticOctree.AddElement(Component, 0);
}

void ULevel::AddBillboardComponentToRender(UBillboardComponent* Component)
{
    if (Component)
    {
        BillboardComponentsToRender.Add(Component);
    }
}

void ULevel::AddActor(AActor* InActor)
{
    if (!InActor)
    {
        return;
    }
    // Rebase ownership to this level
    InActor->SetOuter(this);
    InActor->SetLevel(this);
    LevelActors.push_back(InActor);

    // Register primitives in octree (root and child primitives)
    USceneComponent* Root = InActor->GetRootComponent();
    if (Root && Root->IsA(UPrimitiveComponent::StaticClass()))
    {
        AddToOctree(static_cast<UPrimitiveComponent*>(Root));
    }
    for (UActorComponent* Comp : InActor->GetOwnedComponents())
    {
        if (Comp && Comp->IsA(UPrimitiveComponent::StaticClass()))
        {
            AddToOctree(static_cast<UPrimitiveComponent*>(Comp));
        }
    }
}

void ULevel::NewOctree(const FAABB& OctreeSize)
{
	StaticOctree.NewOctree(OctreeSize);
}

UPrimitiveComponent* ULevel::GetPrimitiveCollided(const FRay& WorldRay, float& ShortestDistance)
{
	return StaticOctree.GetCollidedElement(WorldRay, ShortestDistance);
}

void ULevel::AddTextComponentToRender(UTextRenderComponent* Component)
{
	TextComponentsToRender.push_back(Component);
}



// 1) 유효성 검사: 같은 레벨에 속하고, 삭제 대기 아님, 리스트에 실제로 존재
bool ULevel::IsActorValid(AActor* InActor) const
{
	if (!InActor)
	{
		return false;
	}
	// Actor가 자신의 Level 포인터를 가진다면 가장 빠른 1차 필터
	if (InActor->GetLevel() != this)  // (있다면 사용)
	{
		return false;                    
	}
	if (std::find(LevelActors.begin(), LevelActors.end(), InActor) == LevelActors.end())
	{
		return false;
	}

	if (std::find(ActorsToDelete.begin(), ActorsToDelete.end(), InActor) != ActorsToDelete.end())
	{
		return false;
	}
	return true;
}



/**
 * @brief Level에서 Actor 제거하는 함수
 */
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}
	RemoveFromRenderQueues(InActor);
	// LevelActors 리스트에서 제거
	for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			LevelActors.erase(Iterator);
			break;
		}
	}

	UUIManager& Manager = UUIManager::GetInstance();
	if (InActor == Manager.GetSelectedActor())
	{
		Manager.SetSelectedActor(nullptr);
	}

	// Remove
	SafeDelete(InActor);

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

/**
 * @brief Delete In Next Tick
 */
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		UE_LOG("Level: MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	RemoveFromRenderQueues(InActor);

	UUIManager::GetInstance().SetSelectedActor(nullptr);
	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("Level: Actor Marked For Deletion In Next Tick: %p", InActor);
}

void ULevel::SaveCameraSnapshotFromCamera()
{
	if (!Camera) return;
	FCameraMetadata CameraMetadata;
	CameraMetadata.Location = Camera->GetLocation();
	CameraMetadata.Rotation = Camera->GetRotation(); // 쿼터니언이면 변환해서 저장
	CameraMetadata.Fov = Camera->GetFovY();
	CameraMetadata.Aspect = Camera->GetAspect();
	CameraMetadata.NearZ = Camera->GetNearZ();
	CameraMetadata.FarZ = Camera->GetFarZ();
	SavedCamera = CameraMetadata;
}

void ULevel::ApplySavedCameraSnapshotToCamera()
{
	if (!Camera) return;
	Camera->SetLocation(SavedCamera.Location);
	Camera->SetRotation(SavedCamera.Rotation); // 쿼터니언 API면 거기에 맞춰 Set
	Camera->SetFovY(SavedCamera.Fov);
	Camera->SetAspect(SavedCamera.Aspect);
	Camera->SetNearZ(SavedCamera.NearZ);
	Camera->SetFarZ(SavedCamera.FarZ);
}

void ULevel::SetSavedCameraSnapshot(const FCameraMetadata& In)
{
	SavedCamera = In;
	bSavedCameraDirty = true;  // 더티 플래그 들어왔으니 적용 대기
}

// 더티 플래그를 보고 카메라에 저장된 스냅샷을 적용
void ULevel::TryApplySavedCameraSnapshot()
{
	if (bSavedCameraDirty && Camera)
	{
		ApplySavedCameraSnapshotToCamera();
		bSavedCameraDirty = false;    // 한 번만 적용
	}
}

/**
 * @brief Level에서 Actor를 실질적으로 제거하는 함수
 * 이전 Tick에서 마킹된 Actor를 제거한다
 */
void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("[Level] Processing %zu Pending Deletions", ActorsToDelete.size());

	// 대기 중인 액터들을 삭제
	for (AActor* ActorToDelete : ActorsToDelete)
	{
		if (!ActorToDelete)
			continue;

		// LevelActors 리스트에서 제거
		for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
		{
			if (*Iterator == ActorToDelete)
			{
				LevelActors.erase(Iterator);
				break;
			}
		}

		// Release Memory
		SafeDelete(ActorToDelete);
		UE_LOG("[Level] Actor Deleted: %p", ActorToDelete);
	}

	// Clear TArray
	ActorsToDelete.clear();
	UE_LOG("[Level] All Pending Deletions Processed");
}

void ULevel::RemoveFromRenderQueues(AActor* Owner)
{
	if (!Owner) return;

	StaticMeshComponentsToRender.erase(
		std::remove_if(StaticMeshComponentsToRender.begin(), StaticMeshComponentsToRender.end(),
			[Owner](UStaticMeshComponent* C) { return C && C->GetOuter() == Owner; }),
		StaticMeshComponentsToRender.end());

	TextComponentsToRender.erase(
		std::remove_if(TextComponentsToRender.begin(), TextComponentsToRender.end(),
			[Owner](UTextRenderComponent* C) { return C && C->GetOuter() == Owner; }),
		TextComponentsToRender.end());

	BillboardComponentsToRender.erase(
		std::remove_if(BillboardComponentsToRender.begin(), BillboardComponentsToRender.end(),
			[Owner](UBillboardComponent* C) { return C && C->GetOuter() == Owner; }),
		BillboardComponentsToRender.end());


	//옥트리에서도 제거, 이거 안 하면 dangling 포인터 참조
	for (UActorComponent* Component : Owner->GetOwnedComponents())
	{
		if (Component && Component->IsA(UPrimitiveComponent::StaticClass()))
		{
			StaticOctree.RemoveElement(static_cast<UPrimitiveComponent*>(Component));
		}

	}
}


bool ULevel::IsPendingDeletion(AActor* InActor) const
{
	if (!InActor)
	{
		return false;
	}
	return std::find(ActorsToDelete.begin(), ActorsToDelete.end(), InActor) != ActorsToDelete.end();
}
