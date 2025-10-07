#pragma once

#define DT UTimeManager::GetInstance().GetDeltaTime()

// 싱글톤 패턴 매크로화
// Declare - Implement를 쌍으로 사용해야 한다
#define DECLARE_SINGLETON(ClassName) \
public: \
static ClassName& GetInstance(); \
private: \
ClassName(); \
virtual ~ClassName(); \
ClassName(const ClassName&) = delete; \
ClassName& operator=(const ClassName&) = delete; \
ClassName(ClassName&&) = delete; \
ClassName& operator=(ClassName&&) = delete;

#define IMPLEMENT_SINGLETON(ClassName) \
ClassName& ClassName::GetInstance() \
{ \
static ClassName Instance; \
return Instance; \
}

// UE_LOG Macro
#define UE_LOG(fmt, ...) \
    do { \
        printf(fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog("" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)
//
//// UObject용 싱글톤 Macro
//#define DECLARE_UOBJECT_SINGLETON(ClassName) \
//public: \
//    static ClassName& GetInstance(); \
//	ClassName(); \
//	virtual ~ClassName(); \
//private: \
//    ClassName(const ClassName&) = delete; \
//    ClassName& operator=(const ClassName&) = delete; \
//    ClassName(ClassName&&) = delete; \
//    ClassName& operator=(ClassName&&) = delete;
//
//#define IMPLEMENT_UOBJECT_SINGLETON(ClassName) \
//ClassName& ClassName::GetInstance() \
//{ \
//    static ClassName* Instance = nullptr; \
//    if (!Instance) \
//    { \
//        Instance = NewObject<ClassName>(); \
//    } \
//    return *Instance; \
//}

/*----------------------------------------------------------------------------
	Assertions
----------------------------------------------------------------------------*/

#if _DEBUG
	#define DO_CHECK	1
	#define DO_ENSURE	1
	#define DO_VERIFY	1
#else
	#define DO_CHECK	0
	#define DO_ENSURE	0
	#define DO_VERIFY	0
#endif

/**
 * @brief Compiler type detection macro.
 * @note In this project, only MSVC is officially supported.
 */
#if	  defined(_MSC_VER)
	#define COMPILER_MSVC 1
#elif defined(__clang__)
	#define COMPILER_CLANG 1
#elif defined(__GNUC__)
	#define COMPILER_GCC 1
#endif

/**
 * @brief Breakpoint macro.
 * @note Only MSVC uses __debugbreak(); other compilers use raise(SIGTRAP).
 */
#if defined(COMPILER_MSVC) 
	#define PLATFORM_BREAK() __debugbreak()
#else					   
	#include <csignal>
	#define PLATFORM_BREAK() raise(SIGTRAP)
#endif

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

/**
 * @brief Handles assertion failure by showing a message box and allowing user action.
 *
 * This function constructs a detailed failure message and presents it to the user.
 * The user can choose to:
 * - **Abort**: terminate the program.
 * - **Retry**: trigger a debugger break (if attached).
 * - **Ignore**: continue program execution.
 *
 * @param Expression The failed expression as a string.
 * @param File The source file where the failure occurred.
 * @param Line The line number where the failure occurred.
 * @param Message Optional message providing additional context.
 */
inline void HandleAssertionFailure(const char* Expression, const char* File, uint32 Line, const char* Message)
{
	std::ostringstream Formatter;
	Formatter << "Assertion Failed!"			<< "\n\n"
			  << "Expression: "	<< Expression	<< "\n"
			  << "Message: "	<< Message		<< "\n"
			  << "File: "		<< File			<< "\n"
			  << "Line: "		<< Line			<< "\n"
			  << "(Press Abort to terminate, Retry to debug, or Ignore to continue.)";

	FString FormattedMessage = Formatter.str();

	auto Result = MessageBoxA(
		nullptr,
		FormattedMessage.c_str(),
		"Assertion Failed",
		MB_ABORTRETRYIGNORE | MB_ICONERROR
	);

	switch (Result)
	{
	case IDABORT:
		std::exit(1);
		break;
	case IDRETRY:
		PLATFORM_BREAK();
		break;
	case IDIGNORE:
		break;
	}
}

/**
 * @brief Fatal check that always halts on failure in debug builds.
 *
 * @details
 * - In debug mode (`DO_CHECK`), if `Expression` evaluates to false,
 *   the program displays an assertion message and breaks/terminates.
 * - In release mode, this macro does nothing.
 */
#if DO_CHECK
	#define check(Expression, ...)													\
	do																				\
	{																				\
		if (!(Expression))															\
		{																			\
			HandleAssertionFailure(#Expression, __FILE__, __LINE__, ##__VA_ARGS__); \
		}																			\
	} while (0)																		
#else
	#define check(Expression, ...) (void)0	
#endif

/**
 * @brief Soft check that logs once and continues execution.
 *
 * @details
 * - `ensure()` reports the failure (like `check`) but does **not stop** execution.
 * - It only logs **once** per call site due to the static flag.
 * - Returns `true` if the expression succeeded, `false` otherwise.
 */
#if DO_ENSURE
	#define ensure(Expression, ...)														\
		(!!(Expression) || ([&] {														\
			static bool bIsAlreadyEnsured = false;										\
			if (!bIsAlreadyEnsured)														\
			{																			\
				bIsAlreadyEnsured = true;												\
				HandleAssertionFailure(#Expression, __FILE__, __LINE__, ##__VA_ARGS__); \
			}																			\
			return false;																\
		}()))
#else
	#define ensure(Expression, ...) (!!(Expression))
#endif

/**
 * @brief Verifies expressions that must be evaluated even in release builds.
 *
 * @details
 * - In debug builds (`DO_VERIFY`), behaves like `check()`.
 * - In release builds, evaluates the expression for side effects,
 *   but discards the result using `(void)(Expression)` to avoid warnings.
 */
#if DO_VERIFY
	#define verify(Expression, ...)													\
	do																				\
	{																				\
		if (!(Expression))															\
		{																			\
			HandleAssertionFailure(#Expression, __FILE__, __LINE__, ##__VA_ARGS__); \
		}																			\
	} while(0)
#else
	#define verify(Expression, ...) (void)(Expression)
#endif

