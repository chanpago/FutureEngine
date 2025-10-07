#include "pch.h"
#include "Global/Name.h"
#include "Global/NameTable.h"

FName::FName() : DisplayIndex(0), ComparisonIndex(0), Number(0)
{
}

FName::FName(const FString& Str)
{
	TPair<int32, int32> Indices = FNameTable::GetInstance().FindOrAddName(Str);
	ComparisonIndex = Indices.first;
	DisplayIndex = Indices.second;
	Number = -1;
}

FName::FName(const char* Str) : FName(FString(Str)) { }

/**
* @brief NameTable에서 UniqueName을 만들 때 사용하는 생성자
* 
*/
FName::FName(int32 InDisplayIndex, int32 InComparisonIndex, int32 InNumber)
	: DisplayIndex(InDisplayIndex), ComparisonIndex(InComparisonIndex), Number(InNumber) {}

bool FName::operator==(const FName& Other) const
{
	return ComparisonIndex == Other.ComparisonIndex && Number == Other.Number;
}

int32 FName::Compare(const FName& Other) const
{
	if (*this == Other) { return 0; }

	// ComparisonIndex로 먼저 비교
	if (ComparisonIndex < Other.ComparisonIndex) { return -1; }
	if (ComparisonIndex > Other.ComparisonIndex) { return 1; }

	// ComparisonIndex가 같으면 Number로 비교
	if (Number < Other.Number) { return -1; }
	if (Number > Other.Number) { return 1; }

	return 0;
}

FString FName::ToString() const
{
	FString BaseName = FNameTable::GetInstance().GetDisplayString(DisplayIndex);
	if (Number >= 0)
	{
		return BaseName + "_" + to_string(Number);
	}
	return BaseName;
}



FString FName::ToBaseNameString() const
{
	return FNameTable::GetInstance().GetDisplayString(DisplayIndex);
}

