#include "pch.h"
#include "Global/NameTable.h"
#include "Core/ObjectIterator.h"
#include "Actor/Actor.h"
IMPLEMENT_SINGLETON(FNameTable);

FNameTable::FNameTable()
{
	FindOrAddName("");
}
FNameTable::~FNameTable() = default;

/**
* @brief FString 객체를 받아 풀에 없으면 반환
* @param Str FName으로 등록되었는지 확인할 FString
* @return DisplayIndex, ComparisonIndex
*/
TPair<int32, int32> FNameTable::FindOrAddName(const FString& Str)
{
	FString LowerStr = ToLower(Str);

	int32 ComparisonIndex;
	auto ItComparison = ComparisonMap.find(LowerStr);
	if (ItComparison != ComparisonMap.end())
	{
		ComparisonIndex = ItComparison->second;
	}
	else
	{
		if (!FreeComparisonIndices.Dequeue(ComparisonIndex))
		{
			ComparisonIndex = ComparisonStringPool.size();
			ComparisonStringPool.push_back(LowerStr);
		}
		else
		{
			ComparisonStringPool[ComparisonIndex] = LowerStr;
		}
		ComparisonMap[LowerStr] = ComparisonIndex;
	}

	int32 DisplayIndex;
	auto ItDisplay = DisplayMap.find(Str);
	if (ItDisplay != DisplayMap.end())
	{
		DisplayIndex = ItDisplay->second;
	}
	else
	{
		if (!FreeDisplayIndices.Dequeue(DisplayIndex))
		{
			DisplayIndex = DisplayStringPool.size();
			DisplayStringPool.push_back(Str);
		}
		else
		{
			DisplayStringPool[DisplayIndex] = Str;
		}
		DisplayMap[Str] = DisplayIndex;
	}

	return { ComparisonIndex, DisplayIndex };
}

FName FNameTable::GetUniqueName(const FString& BaseStr)
{
	TPair<int32, int32> Indices = FindOrAddName(BaseStr);
	int32 DisplayIndex = Indices.second;
	int32 ComparisonIndex = Indices.first;

	int32 Number = NextNumberMap[BaseStr];
	NextNumberMap[BaseStr]++;

	return FName(DisplayIndex, ComparisonIndex, Number);
}

FString FNameTable::GetDisplayString(int32 Idx) const
{
	if (Idx >= 0 && Idx < DisplayStringPool.size())
	{
		return DisplayStringPool[Idx];
	}
	static const FString EmptyString = "";
	return EmptyString;
}

FString FNameTable::ToLower(const FString& Str) const
{
	FString LowerStr = Str;
	std::transform(LowerStr.begin(), LowerStr.end(), LowerStr.begin(),
		[](unsigned char C) { return std::tolower(C); });
	return LowerStr;
}
// 이터레이터로 돌아서 액터 목록 순회해서, 액터 이름 빼옴.
// 액터 이름갖고 Pool에서 찾음. -> 인덱스 나와서
// 해당 인덱스만 지움
void FNameTable::Reset()
{
	for (TObjectIterator<AActor> It; It; ++It)
	{
		AActor* Actor = *It;
		if (Actor)
		{
			FString ActorName = Actor->GetClass()->GetName();
			FString LowerName = ToLower(ActorName);

			int32* ComparisonIndex = ComparisonMap.Find(LowerName);
			int32* DisplayIndex = DisplayMap.Find(ActorName);
			if (ComparisonIndex == nullptr)
			{
				continue;
			}
			FreeComparisonIndices.Enqueue(*ComparisonIndex);
			FreeDisplayIndices.Enqueue(*DisplayIndex);

			ComparisonMap.erase(LowerName);
			DisplayMap.erase(ActorName);
			NextNumberMap.erase(ActorName);
		}
	}
}


void FNameTable::PushNumberMapState()
{
	NumberMapStack.Add(NextNumberMap);
	ClearNumMap();
}

void FNameTable::PopNumberMapState()
{
	if (NumberMapStack.IsEmpty())
	{
		return;
	}
	NextNumberMap = NumberMapStack.back();
	NumberMapStack.pop_back();
}
