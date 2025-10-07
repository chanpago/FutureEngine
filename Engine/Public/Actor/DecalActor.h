#pragma once
#include "Actor/Actor.h"

class UBillboardComponent;
class UBoxComponent;
class UDecalComponent;

UCLASS()
class DecalActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(DecalActor, AActor)

public:



private:



	UDecalComponent* Decal					= nullptr;
	UBillboardComponent* SpriteComponent	= nullptr;
	UBoxComponent* BoxComponent				= nullptr;
};

