#pragma once
#include "Core/Object.h"

struct FDuplicatedObject
{
	/** The Duplicated Object */
	UObject* DuplicatedObject;

	FDuplicatedObject()
	{
	}

	FDuplicatedObject(UObject* InDuplicatedObject)
		: DuplicatedObject(InDuplicatedObject)
	{
	}


};
