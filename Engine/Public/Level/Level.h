#pragma once
#include "Core/Object.h"

#include "Editor/Camera.h"
#include "Utility/Metadata.h"
#include "Math/Octree.h"
#include "Components/StaticMeshComponent.h"
class AAxis;
class AGizmo;
class AGrid;
class AActor;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UBillboardComponent;


/**
 * @brief Level Show Flag Enum
 */
//enum class EEngineShowFlags : uint64
//{
//	SF_Primitives = 0x01,
//	SF_BillboardText = 0x10,
//	SF_Bounds = 0x20,
//};


inline uint64 operator&(uint64 lhs, EEngineShowFlags rhs)
{
	return lhs & static_cast<uint64>(rhs);
}



class ULevel : public UObject
{
	DECLARE_CLASS(ULevel, UObject)
public:
	ULevel();
	ULevel(const FString& InName);
	~ULevel() override;

	virtual void Init();
	virtual void Update();
	virtual void Render();
	virtual void Cleanup();

	void UpdateComponentsToRender(AActor* Actor);
	void AddToOctree(UPrimitiveComponent* Component);
	UPrimitiveComponent* GetPrimitiveCollided(const FRay& WorldRay, float& ShortestDistance);
	TArray<AActor*> GetLevelActors() const
	{
		return LevelActors;
	}
	TArray<UStaticMeshComponent*>& GetStaticMeshComponentsToRender()
	{
		return StaticMeshComponentsToRender;
	}
	const TArray<UTextRenderComponent*>& GetTextComponentsToRender() const
	{
		return TextComponentsToRender;
	}
    void AddTextComponentToRender(UTextRenderComponent* Component);
    const TArray<UBillboardComponent*>& GetBillboardComponentsToRender() const { return BillboardComponentsToRender; }
    void AddBillboardComponentToRender(UBillboardComponent* Component);

    // Add an already-created actor to this level (used by PIE duplication)
    void AddActor(AActor* InActor);

	template<typename T, typename... Args>
	T* SpawnActor(const FString& InName = "");

	bool IsActorValid(AActor* InActor) const;
	// 선택된 액터 설정
	// Actor 삭제
	bool DestroyActor(AActor* InActor);

	// 옥트리에서 컴포넌트 삭제
	void RemoveFromOctree(USceneComponent* Component);



	// 지연 삭제를 위한 마킹
	void MarkActorForDeletion(AActor* InActor);


	UCamera* GetCamera() const
	{
		return Camera;
	}

	void SetCamera(UCamera* InCamera)
	{
		Camera = InCamera;
	}

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	TOctree<UPrimitiveComponent, PrimitiveComponentTrait>& GetStaticOctree()
	{
		return StaticOctree;
	}
	// ▼ 카메라 스냅샷 저장/적용
	const FCameraMetadata& GetSavedCameraSnapshot() const
	{
		return SavedCamera;
	}
	void SaveCameraSnapshotFromCamera();
	void NewOctree(const FAABB& OctreeSize);
	void ApplySavedCameraSnapshotToCamera();
	void SetSavedCameraSnapshot(const FCameraMetadata& In);

	// 새로 추가: 더티면 적용(Init/포인터 바인딩/로드 완료 등 '언제든' 호출 가능)
	void TryApplySavedCameraSnapshot();
private:
	// 지연 삭제 처리 함수
	void ProcessPendingDeletions();
	// 렌더 큐에서 해당 액터 컴포넌트 제거
	void RemoveFromRenderQueues(AActor* Owner);
	bool IsPendingDeletion(AActor* InActor) const;
private:
	AActor* SelectedActor = nullptr;
	TArray<AActor*> LevelActors;
	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;


	UCamera* Camera = nullptr;          // 비소유
	FCameraMetadata SavedCamera;        // 직렬화 대상으로 보관
	TOctree<UPrimitiveComponent, PrimitiveComponentTrait> StaticOctree;
	bool bSavedCameraDirty = false;   // 추가
	//렌더러에게 아래의 것들을 그려달라고 주문할 거임

	TArray<UStaticMeshComponent*> StaticMeshComponentsToRender;
	TArray<UTextRenderComponent*> TextComponentsToRender;
    TArray<UBillboardComponent*> BillboardComponentsToRender;


	// 빌보드는 처음에 표시 안하는 게 좋다는 의견이 있어 빌보드만 꺼놓고 출력
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds);
};


template <typename T, typename ... Args>
T* ULevel::SpawnActor(const FString& InName)
{
	T* NewActor = NewObject<T>();

	///////////////////////////////////////////
	NewActor->AddMemoryUsage(sizeof(T));
	///생성자에서 자신의 메모리 설정하게 수정 필요///
	NewActor->SetOuter(this);
	//Outer 설정 시 Outer의 메모리 카운트에 자신의 메모리 합산 작업 수행
	NewActor->SetLevel(this);
	LevelActors.push_back(NewActor);

	AddActor(NewActor);

	if (!InName.empty())
	{
		//NewActor->SetName(InName);
	}
	NewActor->BeginPlay();

	UE_LOG("%s", NewActor->GetName().c_str());

	return NewActor;
}
