#pragma once

class FName
{
public:
	FName();
	FName(const FString& Str);
	FName(const char* Str);
	FName(int32 InDisplayIndex, int32 InComparisonIndex, int32 InNumber);

	bool operator==(const FName& Other) const;
	int32 Compare(const FName& Other) const;

	FString ToString() const;
	FString ToBaseNameString() const;

	int32 GetComparisonIndex() const { return ComparisonIndex; }
	int32 GetDisplayIndex() const { return DisplayIndex; }


private:
	int32 ComparisonIndex;
	int32 DisplayIndex;
	int32 Number;
};
