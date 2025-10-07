#pragma once

#include "Actor/Actor.h"

class UStaticMeshComponent;
class UTextRenderComponent;


UCLASS()
class AStaticMeshActor : public AActor
{
	DECLARE_CLASS(AStaticMeshActor, AActor);

public:
	AStaticMeshActor();
	// 25/09/20 (22:38) Dongmin - 이상적인 방법이 아닐수도 있음
	UStaticMeshComponent* GetStaticMeshComponent() const
	{
		return StaticMeshComponent;
	}



private:
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	UTextRenderComponent* TextComponent = nullptr;
};

