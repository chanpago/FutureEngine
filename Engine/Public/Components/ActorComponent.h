#pragma once
#include "Core/Object.h"

class AActor;

class UActorComponent : public UObject
{
	DECLARE_CLASS(UActorComponent, UObject)

public:
	UActorComponent();
	virtual ~UActorComponent();
	/*virtual void Render(const URenderer& Renderer) const
	{

	}*/

	virtual void BeginPlay();
	virtual void TickComponent();
	virtual void EndPlay();
	virtual void Destroy();

	EComponentType GetComponentType() { return ComponentType; }

	void SetOwner(AActor* InOwner) { Owner = InOwner; }
	AActor* GetOwner() const {return Owner;}

	EComponentType GetComponentType() const { return ComponentType; }

	bool IsComponentTickEnabled() { return bIsComponentTickEnabled; }

	//Duplicate
	virtual void DuplicateSubObjects() override;
	virtual UObject* Duplicate() override;
	virtual void CopyShallow(UObject* Src) override;

protected:
	EComponentType ComponentType;
private:
	AActor* Owner;
	bool bIsComponentTickEnabled; 
};
