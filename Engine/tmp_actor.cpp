#include "pch.h"
#include "Actor/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Mesh/StaticMesh.h"
#include <Actor/StaticMeshActor.h>
#include "Components/TextComponent.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
	USceneComponent* DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
	DefaultSceneComponent->SetOwner(this);
	SetRootComponent(DefaultSceneComponent);
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

//?뚯뒪?몄슜 ?뚮뜑留?肄붾뱶
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
	UActorComponent* NewComponent = nullptr;

	switch (InType)
	{
	case EComponentType::StaticMesh:
		NewComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
		break;
	case EComponentType::Text:
		NewComponent = CreateDefaultSubobject<UTextComponent>("TextComponent");
		break;
	default:
		break;
	}
	if (NewComponent)
	{
		NewComponent->SetOwner(this);
		Cast<USceneComponent>(NewComponent)->SetParentAttachment(InParentComponent);
	}
}

UObject* AActor::Duplicate()
{
	AActor* NewActor = static_cast<AActor*>(GetClass()->CreateDefaultObject());
	if (!NewActor)
	{
		return NewActor;
	}
	NewActor->CopyShallow(this);
	NewActor->SetOuter(this->GetOuter());
	NewActor->SetName(FNameTable::GetInstance().GetUniqueName(this->GetBaseName()));
	NewActor->DuplicateSubObjects();
	return NewActor;
}
void AActor::DuplicateSubObjects()
{
    if (RootComponent)
    {
        RootComponent->Duplicate();
        //RootComponent->SetOuter(this);
        //RootComponent->SetOwner(this);
    }

    for (UActorComponent* Comp : OwnedComponents)
    {
        if (Comp)
        {
            Comp = static_cast<UActorComponent*>(Comp->Duplicate());
            //Comp->SetOuter(this);
            //Comp->SetOwner(this);
        }
    }
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

void AActor::CopyShallow(const UObject* Src)
{
	const AActor* BaseActor = static_cast<const AActor*>(Src);

	this->RootComponent = BaseActor->GetRootComponent();
	this->OwnedComponents = BaseActor->GetOwnedComponents();

	// bIsTickEnabled 
	//UStaticMesh* StaticMesh = UResourceManager::GetInstance().GetStaticMesh("Data/cube-tex.obj");
	//this->SetStaticMesh(StaticMesh);

	//TODO static mesh??異붽???留먯븘? 
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
		// 踰붿쐞 ???쒕뜡 ?꾩튂 
		NewActor->SetActorLocation(OldActor->GetActorLocation());

		// ?꾩쓽???ㅼ???(0.5 ~ 2.0 踰붿쐞) 
		NewActor->SetActorScale3D(OldActor->GetActorScale3D());

		NewOuter->AddToOctree(Cast<UPrimitiveComponent>(NewActor->GetRootComponent()));
		 
	}

	return NewActor;
}


void AActor::Tick()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component)
		{
			Component->TickComponent();
		}
	}
}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}






