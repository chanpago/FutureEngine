#include "pch.h"
#include "Actor/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Mesh/StaticMesh.h"
#include <Actor/StaticMeshActor.h>
#include "Components/TextRenderComponent.h"
#include "Components/BillboardComponent.h" 
#include "Public/Manager/Level/World.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
	USceneComponent* DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
	DefaultSceneComponent->SetOwner(this);
	SetRootComponent(DefaultSceneComponent);

	bTickInEditor = false; bIsTickEnabled = true;
}

AActor::AActor(UObject* InOuter)
{
	SetOuter(InOuter);
}

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}
	SetOuter(nullptr);
	OwnedComponents.clear();
}

//테스트용 렌더링 코드
//void AActor::Render(const URenderer& Renderer)
//{
//	if (RootComponent)
//	{
//		Renderer.UpdateConstant(RootComponent->GetRelativeLocation(), RootComponent->GetRelativeRotation(), RootComponent->GetRelativeScale3D());
//		for (auto& Components : OwnedComponents)
//		{
//			Components->Render(Renderer);
//		}
//	}
//}

void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FVector& InRotation) const
{
    if (RootComponent)
    {
        RootComponent->SetRelativeRotation(InRotation);
    }
}

void AActor::SetActorRotation(const FQuat& InRotation) const
{
    if (RootComponent)
    {
        RootComponent->SetRelativeRotation(InRotation);
    }
}

void AActor::SetActorScale3D(const FVector& InScale) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(InScale);
	}
}

void AActor::SetUniformScale(bool IsUniform)
{
	if (RootComponent)
	{
		RootComponent->SetUniformScale(IsUniform);
	}
}

bool AActor::IsUniformScale() const
{
	if (RootComponent)
	{
		return RootComponent->IsUniformScale();
	}
	return false;
}

void AActor::AddActorComponent(EComponentType InType, USceneComponent* InParentComponent)
{
	// 1. 컴포넌트 타입에 따라 기본 이름을 결정합니다.
	FString BaseName;
	switch (InType)
	{
	case EComponentType::StaticMesh:
		BaseName = "StaticMeshComponent";
		break;
	case EComponentType::Text:
		BaseName = "TextRenderComponent";
		break;
	case EComponentType::Billboard:
		BaseName = "BillboardComponent";
		break;
	default:
		return; // 유효하지 않은 타입
	}

	// 2. 고유한 이름을 생성합니다.
	FString FinalName = BaseName;
	int32 Suffix = 0;
	bool bIsNameUnique = false;
	while (!bIsNameUnique)
	{
		bIsNameUnique = true;
		for (UActorComponent* ExistingComp : GetOwnedComponents())
		{
			if (ExistingComp && ExistingComp->GetName() == FinalName)
			{
				bIsNameUnique = false;
				Suffix++;
				FinalName = BaseName + FString(std::to_string(Suffix).c_str());
				break; // 새로운 이름으로 다시 검사 시작
			}
		}
	}

	// 3. 고유한 이름으로 새 컴포넌트를 생성합니다.
	USceneComponent* NewComponent = nullptr;
	switch (InType)
	{
	case EComponentType::StaticMesh:
		NewComponent = CreateDefaultSubobject<UStaticMeshComponent>(FinalName);
		break;
	case EComponentType::Text:
		NewComponent = CreateDefaultSubobject<UTextRenderComponent>(FinalName);
		break;
	case EComponentType::Billboard:
		NewComponent = CreateDefaultSubobject<UBillboardComponent>(FinalName);
		break;
	}

	// 4. 컴포넌트를 설정하고 옥트리에 등록합니다.
	if (NewComponent)
	{
		NewComponent->SetOwner(this);
		NewComponent->SetParentAttachment(InParentComponent);
		if (!GetRootComponent())
		{
			SetRootComponent(NewComponent);
		}

		ULevel* MyLevel = GetLevel();
		if (MyLevel && NewComponent->IsA(UPrimitiveComponent::StaticClass()))
		{
			MyLevel->AddToOctree(static_cast<UPrimitiveComponent*>(NewComponent));
		}
	}
}

const FVector& AActor::GetActorLocation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeLocation();
}

const FVector& AActor::GetActorRotation() const
{
    assert(RootComponent);
    return RootComponent->GetRelativeRotation();
}

const FQuat& AActor::GetActorRotationQuat() const
{
    assert(RootComponent);
    return RootComponent->GetRelativeRotationQuat();
}

const FVector& AActor::GetActorScale3D() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}

//테스트용
FString AActor::GetStaticMeshName() 
{
	for (auto& Component : GetOwnedComponents())
	{
		if (Component->IsA(UStaticMeshComponent::StaticClass()))
		{
			UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(Component);
			return StaticMeshComponent->GetStaticMeshName();
		}
	}
	return FString();
}

void AActor::SetStaticMesh(UStaticMesh* InStaticMesh)
{
	for (auto& Component : GetOwnedComponents())
	{
		if (Component->IsA(UStaticMeshComponent::StaticClass()))
		{
			UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(Component);
			StaticMeshComponent->SetStaticMesh(InStaticMesh);
		}
	}
}

void AActor::DestroyComponent(USceneComponent* Component)
{
	USceneComponent* Parent = Component->GetParentAttachment();
	TArray<USceneComponent*>& Children = Component->GetChildComponents();
	if (!Parent)
	{
		OwnedComponents.erase(Component);
		//부모가 없는데 자식이 있음(루트 컴포넌트). 자식의 부모 nullptr설정, 형제들이 자식중 하나를 부모로 새로 갖도록 함
		if (Children.Num() > 0)
		{
			SetRootComponent(Children[0]);

			
			for (int Index = 1; Index < Children.Num();)
			{
				Children[Index]->SetParentAttachment(Children[0]);
			}
			Children[0]->SetParentAttachment(nullptr);
		}
		//부모가 없고 자식도 없음. 그냥 루트 nullptr로 설정하고 삭제하면 됨
		else
		{
			SetRootComponent(nullptr);
		}
	}
	else
	{
		GetOwnedComponents().erase(Component);
		//부모가 있고 자식도 있음, 자식들의 부모를 본인의 부모로 설정 필요, 부모의 자식 리스트에서 본인을 제거해야함
		if (Children.Num() > 0)
		{
			//새로운 부모 설정 전에 본인 자식 리스트에서 제거 필요
			TArray<USceneComponent*>& Siblings = Parent->GetChildComponents();
			Siblings.erase(Siblings.begin() + Parent->GetChildComponents().Find(Component));
			//자식들의 부모를 본인의 부모로 설정, 부모를 바꿀때마다 Children의 개수도 줄어들어서
			//결국 모든 자식의 부모를 설정해야 루프에서 벗어남.
			for (int Index = 0; Index < Children.Num();)
			{
				Children[Index]->SetParentAttachment(Parent);
			}
		}
		//부모가 있는데 자식이 없음. 부모의 자식 리스트에서 본인을 제거해야함
		else
		{
			TArray<USceneComponent*>& Siblings = Parent->GetChildComponents();
			Siblings.erase(Siblings.begin() + Parent->GetChildComponents().Find(Component));
		}
	}

	GWorld->GetCurrentLevel()->RemoveFromOctree(Component);

	delete Component;
}

UObject* AActor::Duplicate()
{
	// AActor 복제
	AActor* NewActor = static_cast<AActor*>(GetClass()->CreateDefaultObject());
	if (!NewActor)
	{
		return NewActor;
	}

	// 얕은 복사 추가  // Static Mehes?
	NewActor->CopyShallow(this);
	NewActor->SetName(FNameTable::GetInstance().GetUniqueName(this->GetBaseName()));

	//얕은 복사들을 깊은 복사로 변경
	NewActor->DuplicateSubObjects();
	return NewActor;

	// 위치 회전 크기
}

void AActor::DuplicateSubObjects()
{
	USceneComponent* EditorSceneComp = RootComponent;
	TSet<UActorComponent*> EditorComps = OwnedComponents;

	// Reset current
	RootComponent = nullptr;
	OwnedComponents.clear();

	// Mapping table (old -> new)
	// Mapping table (Editor -> PIE)
	TMap<USceneComponent*, USceneComponent*> SceneMap;

	// Root Component 복사
	if (EditorSceneComp)
	{
		USceneComponent* NewRoot = static_cast<USceneComponent*>(EditorSceneComp->Duplicate());
		if (NewRoot)
		{
			NewRoot->SetOuter(this);
			NewRoot->SetOwner(this);
			RootComponent = NewRoot;
			OwnedComponents.Add(NewRoot);

			//등록
			SceneMap[EditorSceneComp] = NewRoot;
		}
	}

	// Duplicate other components
	for (UActorComponent* Comp : EditorComps)
	{
		// 이미 루트는 순회 했음
		if (!Comp || Comp == EditorSceneComp) continue;

		UActorComponent* NewComp = static_cast<UActorComponent*>(Comp->Duplicate());
		if (!NewComp) continue;
		NewComp->SetOuter(this);
		NewComp->SetOwner(this);
		OwnedComponents.Add(NewComp);

		if (Comp->IsA(USceneComponent::StaticClass()))
		{
			//등록
			SceneMap[static_cast<USceneComponent*>(Comp)] = static_cast<USceneComponent*>(NewComp);
		}
	}

	for (const auto& pair : SceneMap)
	{
		USceneComponent* EditorComp = pair.first;
		USceneComponent* PIEComp	= pair.second;
		USceneComponent* EditorParentComp = EditorComp->GetParentAttachment();

		if (EditorParentComp)
		{
			auto PIEParentComp = SceneMap.find(EditorParentComp);
			if (PIEParentComp != SceneMap.end())
			{
			PIEComp->SetParentAttachment(PIEParentComp->second);
			}
		}

	}


}

void AActor::CopyShallow(UObject* Src)
{
	AActor* BaseActor = static_cast<AActor*>(Src);

	this->RootComponent = BaseActor->GetRootComponent();
	this->OwnedComponents = BaseActor->GetOwnedComponents();
	this->InitPos = BaseActor->InitPos;

	// bIsTickEnabled
	//UStaticMesh* StaticMesh = UResourceManager::GetInstance().GetStaticMesh("Data/cube-tex.obj");
	//this->SetStaticMesh(StaticMesh);

	//TODO static mesh는 추가해 말아?
}

AActor* AActor::DuplicateForTest(ULevel* NewOuter, AActor* OldActor) const
{
	AActor* NewActor = nullptr;

	NewActor = NewOuter->SpawnActor<AStaticMeshActor>();
	UResourceManager& ResourceManager = UResourceManager::GetInstance();

	UStaticMesh* StaticMesh = ResourceManager.GetStaticMesh("Data/cube-tex.obj");
	NewActor->SetStaticMesh(StaticMesh); ;

	if (NewActor)
	{
		// 범위 내 랜덤 위치
		NewActor->SetActorLocation(OldActor->GetActorLocation());

		// 임의의 스케일 (0.5 ~ 2.0 범위)
		NewActor->SetActorScale3D(OldActor->GetActorScale3D());

		NewOuter->AddToOctree(Cast<UPrimitiveComponent>(NewActor->GetRootComponent()));

	}

	return NewActor;
} 

void AActor::Tick()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component && Component->IsComponentTickEnabled())
		{
			Component->TickComponent();
		}
	}
 
	SetActorLocation(GetActorLocation() + InitPos * DT);

}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}
