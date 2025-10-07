#pragma once
#include "Global/CoreTypes.h"

// 전방 선언
class UObject;
class UClass;

/**
 * @brief 언리얼엔진의 UClass를 모방한 클래스 메타데이터 시스템
 *
 * 각 UObject 파생 클래스에 대한 런타임 타입 정보를 저장합니다.
 * - 클래스 이름
 * - 부모 클래스 정보
 * - 클래스 크기
 * - 생성자 함수 포인터
 */
class UClass
{
public:
	// 생성자 함수 포인터 타입 정의
	typedef UObject* (*ClassConstructorType)();

public:
	UClass(const FString& InName, UClass* InSuperClass, size_t InClassSize, ClassConstructorType InConstructor);

	const FString& GetName() const { return ClassName; }
	UClass* GetSuperClass() const { return SuperClass; }
	size_t GetClassSize() const { return ClassSize; }

	bool IsChildOf(const UClass* OtherClass) const;
	UObject* CreateDefaultObject() const;

	//static UClass* FindClass(const FString& InClassName);
	//static void SignUpClass(UClass* InClass);
	//static void PrintAllClasses();

private:
	FString ClassName; // 클래스 이름
	UClass* SuperClass; // 부모 클래스
	size_t ClassSize; // 클래스 크기
	ClassConstructorType Constructor; // 생성자 함수 포인터

	// 전역 클래스 레지스트리
	static TArray<UClass*> AllClasses;
};

/**
 * @brief RTTI 매크로 시스템
 *
 * 언리얼엔진의 UCLASS(), DECLARE_CLASS, IMPLEMENT_CLASS와 유사한 매크로들
 */

// 언리얼 스타일: 클래스 선언 바로 위에서 사용하는 매크로 (여기서는 표시용, 실동작은 GENERATED_BODY에서 처리)
#define UCLASS()

// 클래스 본문 내부에서 사용하는 매크로 (friend 등 내부 선언 처리)
#define GENERATED_BODY() \
public: \
    friend class UClass; \
    friend class UObject;

// 클래스 선언부에 사용하는 매크로
#define DECLARE_CLASS(ClassName, SuperClassName) \
public: \
    typedef ClassName ThisClass; \
    typedef SuperClassName Super; \
    static UClass* StaticClass(); \
    virtual UClass* GetClass() const; \
    static UObject* CreateDefaultObject##ClassName();

// 클래스 구현부에 사용하는 매크로
#define IMPLEMENT_CLASS(ClassName, SuperClassName) \
    UClass* ClassName::StaticClass() \
    { \
        /* 정적 지역 변수를 사용하여 UClass 객체를 자동 관리 */ \
        static UClass Instance( \
            FString(#ClassName), \
            SuperClassName::StaticClass(), \
            sizeof(ClassName), \
            &ClassName::CreateDefaultObject##ClassName \
        ); \
        return &Instance; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    UObject* ClassName::CreateDefaultObject##ClassName() \
    { \
        return new ClassName(); \
    }


// 추상 클래스에 사용하는 매크로 (기본 객체를 생성하지 않음)
#define IMPLEMENT_ABSTRACT_CLASS(ClassName, SuperClassName) \
    UClass* ClassName::StaticClass() \
    { \
        static UClass Instance( \
            FString(#ClassName), \
            SuperClassName::StaticClass(), \
            sizeof(ClassName), \
            nullptr \
        ); \
        return &Instance; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    }

// UObject의 기본 매크로 (다른 클래스들의 베이스)
#define IMPLEMENT_CLASS_BASE(ClassName) \
    UClass* ClassName::StaticClass() \
    { \
        static UClass Instance( \
            FString(#ClassName), \
            nullptr, \
            sizeof(ClassName), \
            &ClassName::CreateDefaultObject##ClassName \
        ); \
        return &Instance; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    UObject* ClassName::CreateDefaultObject##ClassName() \
    { \
        return new ClassName(); \
    }
