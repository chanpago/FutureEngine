#pragma once

using std::string;
using std::wstring;

template <typename T>
static void SafeDelete(T& InDynamicObject)
{
	delete InDynamicObject;
	InDynamicObject = nullptr;
}

static FString WideStringToString(const wstring& InString)
{
	int32 ByteNumber = WideCharToMultiByte(CP_UTF8, 0,
	                                     InString.c_str(), -1, nullptr, 0, nullptr, nullptr);

	FString OutString(ByteNumber, 0);

	WideCharToMultiByte(CP_UTF8, 0,
	                    InString.c_str(), -1, OutString.data(), ByteNumber, nullptr, nullptr);

	return OutString;
}

static wstring StringToWideString(const FString& str)
{
	if (str.empty()) {
		return std::wstring();
	}
	
	int32 ByteNumber = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);

	std::wstring OutString(ByteNumber, 0);

	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &OutString[0], ByteNumber);

	return OutString;
}
/**
 * @brief CP949 (ANSI) 타입의 문자열을 UTF-8 타입으로 변환하는 함수
 * CP949 To UTF-8 직변환 함수가 제공되지 않아 UTF-16을 브릿지로 사용한다
 * @param InANSIString ANSI 문자열
 * @return UTF-8 문자열
 */
static string ConvertCP949ToUTF8(const char* InANSIString)
{
	if (InANSIString == nullptr || InANSIString[0] == '\0')
	{
		return "";
	}

	// CP949 -> UTF-8 변환
	int WideCharacterSize = MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, nullptr, 0);
	if (WideCharacterSize == 0)
	{
		return "";
	}

	wstring WideString(WideCharacterSize, 0);
	MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, WideString.data(), WideCharacterSize);

	// UTF-16 -> UTF-8 변환
	int UTF8Size = WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (UTF8Size == 0)
	{
		return "";
	}

	string UTF8String(UTF8Size, 0);
	WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, UTF8String.data(), UTF8Size, nullptr, nullptr);

	if (!UTF8String.empty() && UTF8String.back() == '\0')
	{
		UTF8String.pop_back();
	}

	return UTF8String;
}
