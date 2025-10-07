#include "pch.h"
#include "Core/Object.h"
#include "Core/EngineStatics.h"

uint32 UEngineStatics::NextUUID = 0;
TArray<UObject*> GUObjectArray;

IMPLEMENT_CLASS_BASE(UObject)

UObject::UObject() : Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	//Name = FNameTable::GetInstance().GetUniqueName("");
	Name = FNameTable::GetInstance().GetUniqueName(GetClass()->GetName());

	GUObjectArray.push_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}

UObject::UObject(const FString& InString) : Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	Name = FNameTable::GetInstance().GetUniqueName(InString);

	GUObjectArray.push_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}
// 소멸자 추가
UObject::~UObject()
{
	// Outer 집계 정리
	if (Outer)
	{
		Outer->RemoveMemoryUsage(AllocatedBytes, AllocatedCounts);
		Outer = nullptr;
	}
	RemoveFromGlobalObjectArray(this);
	InternalIndex = (uint32)-1;
}

void UObject::DuplicateSubObjects()
{

}

UObject* UObject::Duplicate()
{
	UClass* Class = GetClass();
	UObject* NewObject = Class ? Class->CreateDefaultObject() : nullptr;
	if (!NewObject)
	{
		return nullptr;
	}

	CopyShallow(this);

	return NewObject;
}

void UObject::CopyShallow(UObject* Src)
{
	this->Outer = Src->Outer;
	this->SetName(Src->GetName());
	//this->UUID= Src->GetUUID(); 
}
 

void UObject::SetOuter(UObject* InObject)
{
	// 자기 자신으로의 재설정 방지
	if (Outer == InObject) 
	{
		return;
	}
	// 기존 Outer가 있다면 해당 오브젝트에서 메모리 관리 제거
	if (Outer)
	{
		Outer->RemoveMemoryUsage(AllocatedBytes, AllocatedCounts);
	}

	// 새로운 Outer 설정 후 새로운 Outer에서 메모리 관리
	Outer = InObject;
	if (Outer)
	{
		Outer->AddMemoryUsage(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 InBytes, uint32 InCount)
{
	AllocatedBytes += InBytes;
	AllocatedCounts += InCount;

	if (Outer)
	{
		Outer->AddMemoryUsage(InBytes);
	}
}

void UObject::RemoveMemoryUsage(uint64 InBytes, uint32 InCount)
{
	if (AllocatedBytes >= InBytes)
	{
		AllocatedBytes -= InBytes;
	}
	if (AllocatedCounts >= InCount)
	{
		AllocatedCounts -= InCount;
	}

	if (Outer)
	{
		Outer->RemoveMemoryUsage(InBytes);
	}
}

bool UObject::IsA(const UClass* InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass()->IsChildOf(InClass);
}

void RemoveFromGlobalObjectArray(UObject* Obj)
{
	if (!Obj) return;
	uint32 idx = Obj->InternalIndex; // private이면 getter 추가하거나 friend 처리
	if (idx < GUObjectArray.size() && GUObjectArray[idx] == Obj) {
		UObject* last = GUObjectArray.back();
		GUObjectArray[idx] = last;
		last->InternalIndex = idx;
		GUObjectArray.pop_back();
	}
	else {
		// 인덱스가 꼬였을 때 방어적으로 선형 제거
		auto it = std::find(GUObjectArray.begin(), GUObjectArray.end(), Obj);
		if (it != GUObjectArray.end()) {
			*it = GUObjectArray.back();
			(*it)->InternalIndex = static_cast<uint32>(it - GUObjectArray.begin());
			GUObjectArray.pop_back();
		}
	}
}
