#pragma once
#include "Core/Object.h"
#include <type_traits>
/*
* 왜 UObject를 탐색하는 건가?
* -> 에셋/리소스, CDO, 에디터 전용 오브젝트 등. AActor로 잡지 못하는 것이 있음.
* (AActor 순회는 Level에 존재하는 런타임 엔티티만 빠르게 보고싶을 때 사용)
* - 에디터 툴, 애셋 일괄 처리, 레퍼런스 감사(Reference Audit), 리다이렉터 정리, 머터리얼/텍스처 스캔
*   같은 작업은 UObject를 대상으로 해야한다.
* - UStaticMesh, UMaterialInterface, UFont, USkeleton… 이런 건 액터 트리로 못 찾음.
* - 에디터 시작 직후, 월드 열리기 전/닫힌 후, GC/Hot-Reload/PIE 전환 등 월드가 불확실한 타이밍에도 동작해야 하는 툴들이 많다.
* - 가장 중요한 이유로 Garbage Collector가 이를 사용한다.
*/

/*
* 전역 오브젝트 컨테이너(GUObjectArray)를 인덱스(CurrentIndex)로 순회하면서
* TObject 타입에 해당하는 애들만 필터링해서 멈추는 Iterator
*/
template<typename TObject>
class FObjectIterator
{
	// 반드시 UObject 파생 타입만 돌도록 컴파일 타임에 보증
	static_assert(std::is_base_of<UObject, TObject>::value,
		"FObjectIterator requires a UObject-derived type.");
public:
	/*
	* -1 에서 시작해, 첫 유효 객체를 찾을 때까지 한 번 전진
	* 생성 직후 곧바로 *It가 첫 번째 매칭 객체 가리킴
	*/
	FObjectIterator()
		: CurrentIndex(-1)
	{
		AdvanceToNextValidObject();
	}
	// 현재 가리키는 객체 TObject*로 반환
	TObject* operator*() const
	{
		const int32 Count = static_cast<int32>(GUObjectArray.size());
		return (CurrentIndex >= 0 && CurrentIndex < Count)
			? static_cast<TObject*>(GUObjectArray[CurrentIndex])
			: nullptr;
	}
	// 내부 접근자
	TObject* operator->() const { return **this; }

	// while(It) 사용 가능하도록 변환 연산자
	explicit operator bool() const
	{
		const int32 Count = static_cast<int32>(GUObjectArray.size());
		return CurrentIndex >= 0 && CurrentIndex < Count;
	}
	int32 GetIndex() const { return CurrentIndex; }

	// 다음 인덱스로 이동, 유효한 타입이 나올 때까지 전진
	FObjectIterator& operator++()
	{
		++CurrentIndex;
		AdvanceToNextValidObject();
		return *this;
	}
private:
	// 유효한 타입이 나올 때까지 전진
	void AdvanceToNextValidObject()
	{
		const int32 Count = static_cast<int32>(GUObjectArray.size());

		while (true)
		{
			if (CurrentIndex < 0) { CurrentIndex = 0; }
			if (CurrentIndex >= Count) break;

			UObject* Obj = GUObjectArray[CurrentIndex];
			if (Obj && Obj->IsA(TObject::StaticClass()))
			{
				// 유효 타입 발견
				break;
			}
			++CurrentIndex;
		}
	}

private:
	int32 CurrentIndex;
};

// UE 사용감 맞추는 별칭
template<typename T>
using TObjectIterator = FObjectIterator<T>;
