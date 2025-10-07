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
