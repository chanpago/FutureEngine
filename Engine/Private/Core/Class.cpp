#include "pch.h"
#include "Core/Class.h"

#include "Core/Object.h"

using std::stringstream;

// 전역 클래스 레지스트리 초기화
TArray<UClass*> UClass::AllClasses;

/**
 * @brief UClass Constructor
 * @param InName Class 이름
 * @param InSuperClass Parent Class
 * @param InClassSize Class Size
 * @param InConstructor 생성자 함수 포인터
 */
UClass::UClass(const FString& InName, UClass* InSuperClass, size_t InClassSize, ClassConstructorType InConstructor)
	: ClassName(InName), SuperClass(InSuperClass), ClassSize(InClassSize), Constructor(InConstructor)
{
	//UE_LOG("UClass: 클래스 등록: %s", ClassName.c_str());
}

/**
 * @brief 이 클래스가 지정된 클래스의 하위 클래스인지 확인
 * @param OtherClass 확인할 클래스
 * @return 하위 클래스이거나 같은 클래스면 true
 */
bool UClass::IsChildOf(const UClass* OtherClass) const
{
	if (!OtherClass)
	{
		return false;
	}

	// 자기 자신과 같은 클래스인 경우
	if (this == OtherClass)
	{
		return true;
	}

	// 부모 클래스들을 거슬러 올라가면서 확인
	const UClass* CurrentClass = this;
	while (CurrentClass)
	{
		if (CurrentClass == OtherClass)
		{
			return true;
		}

		CurrentClass = CurrentClass->SuperClass;
	}

	return false;
}

/**
 * @brief 새로운 인스턴스 생성
 * @return 생성된 객체 포인터
 */
UObject* UClass::CreateDefaultObject() const
{
	if (Constructor)
	{
		return Constructor();
	}

	return nullptr;
}

///** UClass 메모리 누수문제로 잠시 기능 제거
// * @brief 클래스 이름으로 UClass 찾기
// * @param InClassName 찾을 클래스 이름
// * @return 찾은 UClass 포인터 (없으면 nullptr)
// */
//UClass* UClass::FindClass(const FString& InClassName)
//{
//	for (UClass* Class : AllClasses)
//	{
//		if (Class && Class->GetName() == InClassName)
//		{
//			return Class;
//		}
//	}
//
//	return nullptr;
//}
//
///**
// * @brief 모든 UClass 등록
// * @param InClass 등록할 UClass
// */
//void UClass::SignUpClass(UClass* InClass)
//{
//	if (InClass)
//	{
//		AllClasses.push_back(InClass);
//		//UE_LOG("UClass: Class registered: %s (Total: %llu)", InClass->GetName().c_str(), AllClasses.size());
//	}
//}
//
///**
// * @brief 등록된 모든 클래스 출력
// * For Debugging
// */
//void UClass::PrintAllClasses()
//{
//	UE_LOG("=== Registered Classes (%llu) ===", AllClasses.size());
//
//	for (size_t i = 0; i < AllClasses.size(); ++i)
//	{
//		UClass* Class = AllClasses[i];
//
//		stringstream ss;
//		ss << Class->GetName();
//
//		if (Class)
//		{
//			ss << "[" << i << "] " << Class->GetName()
//				<< " (Size: " << Class->GetClassSize() << " bytes)";
//
//			if (Class->GetSuperClass())
//			{
//				ss << " -> " << Class->GetSuperClass()->GetName();
//			}
//			else
//			{
//				ss << " (Base Class)";
//			}
//			UE_LOG("%s", ss.str().c_str());
//		}
//	}
//
//	UE_LOG("================================");
//}
