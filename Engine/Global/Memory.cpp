#include "pch.h"
#include "Global/Memory.h"

uint32 TotalAllocationBytes = 0;
uint32 TotalAllocationCount = 0;

void* operator new(size_t size)
{
	TotalAllocationCount++;
	TotalAllocationBytes += static_cast<uint32>(size);
	AllocHeader* header = (AllocHeader*)malloc(sizeof(AllocHeader) + size);
	header->size = size;
	return (void*)(header + 1);
}

void operator delete(void* memory) noexcept
{
	if (!memory) return;

	AllocHeader* header = ((AllocHeader*)memory) - 1;
	size_t size = header->size;

	if (TotalAllocationCount > 0)
	{
		--TotalAllocationCount;
	}

	if (TotalAllocationBytes >= size)
	{
		TotalAllocationBytes -= static_cast<uint32>(size);
	}

	free(header);
}

