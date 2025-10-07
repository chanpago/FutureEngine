#include "pch.h"
#include "ArchiveFileWriter.h"

FArchiveFileWriter::FArchiveFileWriter(const FString& BinFilePath)
{
	CheckDirectory(BinFilePath);
	
}

void FArchiveFileWriter::Serialize(void* Data, uint64 Length)
{
	if (FilePointer)
	{
		fwrite(Data, 1, Length, FilePointer);
	}
}

bool FArchiveFileWriter::FileOpen(const FString& FilePath)
{
	fopen_s(&FilePointer, FilePath.c_str(), "ab");
	return FilePointer != nullptr;
}

void FArchiveFileWriter::FileClose()
{
	if (FilePointer)
	{		
		if (fclose(FilePointer) != 0)
		{
			// 여전히 열려있으면 로그 출력
			UE_LOG("File Close Fail");
		}
		FilePointer = nullptr;
	}
}

bool FArchiveFileWriter::CheckDirectory(const FString& FilePath)
{
	filesystem::path FilePathName(FilePath.c_str());
	filesystem::path ParentDirectory = FilePathName.parent_path();
	
	if (!filesystem::exists(ParentDirectory))
	{
		try
		{
			filesystem::create_directories(ParentDirectory);
		}
		catch(const filesystem::filesystem_error& ex)
		{
			UE_LOG("디렉토리 생성 실패 : %s", FilePath.c_str());
			return false;
		}
	}

	return true;
}

bool FArchiveFileWriter::IsFileExist(const FString& FilePath)
{
	filesystem::path FilePathName(FilePath.c_str());

	if (!filesystem::exists(FilePathName))
	{
		UE_LOG(".Bin 파일이 존재하지 않습니다.");
		return false;
	}

	return true;
}

bool FArchiveFileWriter::IsBinOld(const FString& OriginalFile, const FString& BinFile, EFileFormat Format)
{
	// 원본이 존재하지 않고 .bin이 존재하면 bin파일 삭제
	if (!IsFileExist(OriginalFile) && IsFileExist(BinFile))
	{
		filesystem::remove(BinFile);
		return false;
	}


	try
	{
		auto BinFileTime = filesystem::last_write_time(BinFile);
		auto ObjFileTime = filesystem::last_write_time(OriginalFile);

		if (ObjFileTime > BinFileTime)
		{
			fopen_s(&FilePointer, BinFile.c_str(), "wb");
			UE_LOG("%s 갱신", BinFile.c_str());
			return true;
		}
		
	}
	// try 구문이 실패하면 무조건 bin 작성
	catch (const filesystem::filesystem_error& ex)
	{
		fopen_s(&FilePointer, BinFile.c_str(), "wb");
		return true;
	}

	return false;
}
