#pragma once
#include "Core/Object.h"
#include "Components/SceneComponent.h"
#include "level/Level.h"

class ULevel;
class UStaticMesh;
/**
 * @brief Level에서 렌더링되는 UObject 클래스
 * UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
 */
class AActor : public UObject
{
	DECLARE_CLASS(AActor, UObject)
public:
	AActor();
	AActor(UObject* InOuter);
	virtual ~AActor() override;
	// ----- Transform -----
	void SetActorLocation(const FVector& InLocation) const;
    void SetActorRotation(const FVector& InRotation) const;
    void SetActorRotation(const struct FQuat& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);

	bool IsUniformScale() const;


	template <typename T>
	T* CreateDefaultSubobject(const FString& InName);

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick();

	// ----- Components -----
	// Getter & Setter
	USceneComponent* GetRootComponent() const
	{
		return RootComponent;
	}
	TSet<UActorComponent*>& GetOwnedComponents()
	{
		return OwnedComponents;
	}
	void SetRootComponent(USceneComponent* InOwnedComponents)
	{
		RootComponent = InOwnedComponents;
	}
	bool IsActorTickEnabled() { return bIsTickEnabled; };
	bool IsTickInEditor() { return bTickInEditor; };

	// ----- Actor state -----
	const FVector& GetActorLocation() const;
    const FVector& GetActorRotation() const;
    const FQuat& GetActorRotationQuat() const;
	const FVector& GetActorScale3D() const;

	void AddActorComponent(EComponentType InType, USceneComponent* InParentComponent);

	// ----- Level back-pointer (비소유) -----
	ULevel* GetLevel() const
	{
		return OwnerLevel;
	}
	void SetLevel(ULevel* InLevel) // 레벨에서만 호출
	{
		OwnerLevel = InLevel;
	}

	//테스트용
	FString GetStaticMeshName();
	void SetStaticMesh(UStaticMesh* InStaticMesh);
	void DestroyComponent(USceneComponent* Component);

	//PIE
	virtual void DuplicateSubObjects() override;
	virtual UObject* Duplicate() override;

	virtual AActor* DuplicateForTest(class ULevel* NewOuter, AActor* OldActor) const;
	virtual void CopyShallow(UObject* Src) override;

	void SetInitPos(FVector Pos) { InitPos = Pos; }
	FVector GetInitPos() { return InitPos; }
private:
	USceneComponent* RootComponent = nullptr;
	TSet<UActorComponent*> OwnedComponents;
	ULevel* OwnerLevel = nullptr; 	// 현재 자신이 속한 레벨(비소유, 생명주기 소유권 없음)

	bool bIsTickEnabled;
	bool bTickInEditor;

	FVector InitPos; 
};

template <typename T>
T* AActor::CreateDefaultSubobject(const FString& InName)
{
	T* NewComponent = NewObject<T>();

	///////////////////////////////////////////
	NewComponent->AddMemoryUsage(sizeof(T));
	///생성자에서 자신의 메모리 설정하게 수정 필요///
	NewComponent->SetOuter(this);
	//Outer 설정 시 Outer의 메모리 카운트에 자신의 메모리 합산 작업 수행

	////////////////////////////////////
	NewComponent->SetOwner(this);
	///RTTI로 Owner와 Outer 동일화 필////

	NewComponent->SetName(InName);
	OwnedComponents.insert(NewComponent);

	return NewComponent;
}
