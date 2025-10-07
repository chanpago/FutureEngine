#pragma once
#include "Utility/ArchiveFileReader.h"
#include "Utility/ArchiveFileWriter.h"

class IFileManager
{
	DECLARE_SINGLETON(IFileManager);

public:
	FArchive* CreateFileWriter(const FString& BinFilePath);
	FArchive* CreateFileReader(const FString& BinFilePath);
};
