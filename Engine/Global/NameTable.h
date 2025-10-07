#pragma once

class FNameTable
{
	DECLARE_SINGLETON(FNameTable);

public:
	TPair<int32, int32> FindOrAddName(const FString& Str);
	FName GetUniqueName(const FString& BaseStr);

	FString GetDisplayString(int32 Idx) const;

	void Reset();
	void ClearNumMap() { NextNumberMap.clear();  }
private:
	FString ToLower(const FString& Str) const;
	// TODO - 현재는 Resize가 없고, 지워진 인덱스는 재사용하는 방식임
	// 
	// 만약에 이 엔진을 계속 사용한다면
	// TArray로 문자열 정보를 저장하는 것이 아닌
	// TMap<int32, FString> 으로 저장해서 인덱스가 비어있는 경우를
	// 체크하는 방법도 고려하는 것이 좋아보입니다.

	TArray<FString> ComparisonStringPool; // 아래 맵의 인덱스로 액터만 지우도록
	TArray<FString> DisplayStringPool;

	TQueue<int32> FreeComparisonIndices;
	TQueue<int32> FreeDisplayIndices;

	// 이름, 인덱스
	TMap<FString, int32> ComparisonMap;
	TMap<FString, int32> DisplayMap;

	// 이름, 중복된 base string에 대한 넘버링
	TMap<FString, int32> NextNumberMap;
};
