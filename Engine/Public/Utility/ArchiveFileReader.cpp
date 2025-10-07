#include "pch.h"
#include "ArchiveFileReader.h"
#include "FileManager.h"

FArchiveFileReader::FArchiveFileReader(const FString& FilePath)
{
	// 이진파일 읽기
	fopen_s(&FilePointer, FilePath.c_str(), "rb");

	if (FilePointer == nullptr)
	{
		assert("FArchiveFileReader File Open 실패");
	}
}

void FArchiveFileReader::Serialize(void* Data, uint64 Length)
{
	if (FilePointer)
	{
		fread(Data, 1, Length, FilePointer);
	}
}

bool FArchiveFileReader::FileOpen(const FString& FilePath)
{
	return false;
}

void FArchiveFileReader::FileClose()
{
	if (FilePointer)
	{
		if (fclose(FilePointer) != 0)
		{
			assert("File Close Fail");
		}
		FilePointer = nullptr;
	}
}

bool FArchiveFileReader::IsFileExist(const FString& FilePath)
{
	filesystem::path FilePathName(FilePath.c_str());

	if (!filesystem::exists(FilePathName))
	{
		UE_LOG(".Bin 파일이 존재하지 않습니다.");
		return false;
	}

	return true;
}

bool FArchiveFileReader::IsBinOld(const FString& OriginalFile, const FString& BinFile, EFileFormat Format)
{
	return false;
}
