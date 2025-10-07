#include "pch.h"
#include "Actor/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
{
	// AActor 생성자가 만든 기본 루트 컴포넌트를 먼저 파괴합니다.                  
	if (GetRootComponent())
	{
		// OwnedComponents 목록에서도 제거해야 합니다.                      
		USceneComponent* DefaultRoot = GetRootComponent();
		GetOwnedComponents().erase(DefaultRoot);
		delete DefaultRoot;
		SetRootComponent(nullptr);
	}

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	StaticMeshComponent->SetOwner(this);
	SetRootComponent(StaticMeshComponent);

}
