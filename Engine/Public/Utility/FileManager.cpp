#include "pch.h"
#include "FileManager.h"

IMPLEMENT_SINGLETON(IFileManager);

IFileManager::IFileManager()
{

}

IFileManager::~IFileManager()
{

}

FArchive* IFileManager::CreateFileWriter(const FString& BinFilePath)
{
	FArchive* Writer = new FArchiveFileWriter(BinFilePath);
	return Writer;
}

FArchive* IFileManager::CreateFileReader(const FString& BinFilePath)
{
	FArchive* Reader = new FArchiveFileReader(BinFilePath);
	return Reader;
}
