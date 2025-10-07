#pragma once
#include "Class.h"
#include "Global/Memory.h"


UCLASS()
class UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObject, UObject)

public:
	// Special Member Function
	UObject();
	explicit UObject(const FString& InString);
	virtual ~UObject();

	virtual void DuplicateSubObjects();
	virtual UObject* Duplicate();
	virtual void CopyShallow(UObject* Src);
	 
	// Getter & Setter
	FString GetName() const
	{
		return Name.ToString();
	}
	FString GetBaseName() const
	{
		return Name.ToBaseNameString();
	}
	UObject* GetOuter() const
	{
		return Outer;
	}
	uint32 GetUUID() const
	{
		return UUID;
	}

	void SetName(const FName& InName)
	{
		Name = InName;
	}
	void SetOuter(UObject* InObject);

	void AddMemoryUsage(uint64 InBytes, uint32 InCount = 1);
	void RemoveMemoryUsage(uint64 InBytes, uint32 InCount = 1);

	uint64 GetAllocatedBytes() const
	{
		return AllocatedBytes;
	}
	uint32 GetAllocatedCount() const
	{
		return AllocatedCounts;
	}

	bool IsA(const UClass* InClass) const;
	// (선택) 내부 인덱스 확인용
	uint32 GetInternalIndex() const
	{
		return InternalIndex;
	}

private:
	uint32 UUID = UINT32_MAX;
	uint32 InternalIndex = UINT32_MAX;
	FName Name;
	// 소유자(컨테이너) 의미. 어떤 UObject가 다른 UObject 안에 속해있다는 것을 표현함.
	UObject* Outer = nullptr;
	/*
	* Ex) Outer의 의미 예시
	*	World (Outer = null)
	*		└─ Level_A (Outer = World)
	*		   ├─ Actor_1 (Outer = Level_A)
	*		   │  └─ Component_X (Outer = Actor_1)
	*		   └─ Actor_2 (Outer = Level_A)
	*/

	uint64 AllocatedBytes = 0;
	uint32 AllocatedCounts = 0;

	friend void RemoveFromGlobalObjectArray(UObject* Obj);
};

extern TArray<UObject*> GUObjectArray;

template <typename T>
T* NewObject()
{
	T* NewObject = new T();
	NewObject->SetName(FNameTable::GetInstance().GetUniqueName(NewObject->GetClass()->GetName()));

	return NewObject;
}

// UObject Cast
template<typename T>
T* Cast(UObject* Object)
{
	if (!Object)
		return nullptr;

	if (Object->IsA(T::StaticClass()))
		return static_cast<T*>(Object);

	return nullptr;
}
