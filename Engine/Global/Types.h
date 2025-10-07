#pragma once

//STL Redefine
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <list>
#include <string>
#include <array>
#include <algorithm>

/** UE5 스타일 기본 타입 정의 */
using uint8 = std::uint8_t;
using int8 = std::int8_t;
using uint16 = std::uint16_t;
using int16 = std::int16_t;
using uint32 = std::uint32_t;
using int32 = std::int32_t;
using uint64 = std::uint64_t;
using int64 = std::int64_t;

/** UE5 스타일 문자열 정의 */
using FString = std::string;
using FWstring = std::wstring;

/** TPair 구현 */
template<typename T1, typename T2>
using TPair = std::pair<T1, T2>;

/** TStaticArray 구현 (고정 크기 배열) */
template<typename T, size_t N>
using TStaticArray = std::array<T, N>;

/** TArray 구현 */
template<typename T, typename AllocatorType = std::allocator<T>>
class TArray : public std::vector<T, AllocatorType>
{
public:
	using std::vector<T, AllocatorType>::vector; /** 생성자 상속 */

    /** 요소 추가 */
    int32 Add(const T& Item)
    {
        this->push_back(Item);
        return static_cast<int32>(this->size() - 1);
    }

    template<typename... Args>
    int32 Emplace(Args&&... args)
    {
        this->emplace_back(std::forward<Args>(args)...);
        return static_cast<int32>(this->size() - 1);
    }

    /** 고유 요소만 추가 */
    int32 AddUnique(const T& Item)
    {
        auto it = std::find(this->begin(), this->end(), Item);
        if (it == this->end())
        {
            return Add(Item);
        }
        return static_cast<int32>(std::distance(this->begin(), it));
    }

    /** 배열 병합 */
    void Append(const TArray<T>& Other)
    {
        this->insert(this->end(), Other.begin(), Other.end());
    }

    /** 삽입 */
    void Insert(const T& Item, int32 Index)
    {
        this->insert(this->begin() + Index, Item);
    }

    /** 제거 */
    void RemoveAt(int32 Index)
    {
        this->erase(this->begin() + Index);
    }

    bool Remove(const T& Item)
    {
        auto it = std::find(this->begin(), this->end(), Item);
        if (it != this->end())
        {
            this->erase(it);
            return true;
        }
        return false;
    }

    int32 RemoveAll(const T& Item)
    {
        auto oldSize = this->size();
        this->erase(std::remove(this->begin(), this->end(), Item), this->end());
        return static_cast<int32>(oldSize - this->size());
    }

    /** 크기 관련 */
    int32 Num() const
    {
        return static_cast<int32>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        this->clear();
    }

    // 보유 용량(capacity)을 실제 크기(Num)에 맞춰 축소
    void Shrink()
    {
        this->shrink_to_fit();
    }

    void Reserve(int32 Capacity)
    {
        this->reserve(static_cast<size_t>(Capacity));
    }

    void SetNum(int32 NewSize)
    {
        this->resize(static_cast<size_t>(NewSize));
    }

    void SetNum(int32 NewSize, const T& DefaultValue)
    {
        this->resize(static_cast<size_t>(NewSize), DefaultValue);
    }

    /** 접근 */
    T& Last()
    {
        return this->back();
    }

    const T& Last() const
    {
        return this->back();
    }

    /** Stack 기능 - std::stack 대체 */
    void Push(const T& Item)
    {
        this->push_back(Item);
    }

    T Pop()
    {
        T Item = this->back();
        this->pop_back();
        return Item;
    }

    /** 검색 */
    int32 Find(const T& Item) const
    {
        auto it = std::find(this->begin(), this->end(), Item);
        return (it != this->end()) ? static_cast<int32>(std::distance(this->begin(), it)) : -1;
    }

    bool Contains(const T& Item) const
    {
        return Find(Item) != -1;
    }

    /** 정렬 */
    void Sort()
    {
        std::sort(this->begin(), this->end());
    }

    template<typename Predicate>
    void Sort(Predicate Pred)
    {
        std::sort(this->begin(), this->end(), Pred);
    }
};

/** TSet - 해시 기반 집합 */
template<typename ElementType, typename Hasher = std::hash<ElementType>, typename Equal = std::equal_to<ElementType>, typename Alloc = std::allocator<ElementType>>
class TSet : public std::unordered_set<ElementType, Hasher, Equal, Alloc>
{
public:
	using std::unordered_set<ElementType, Hasher, Equal, Alloc>::unordered_set;

    /** 요소 추가 */
    void Add(const ElementType& Item)
    {
        this->insert(Item);
    }

    /** 제거 */
    bool Remove(const ElementType& Item)
    {
        return this->erase(Item) > 0;
    }

    /** 크기 관련 */
    int32 Num() const
    {
        return static_cast<int32>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        this->clear();
    }

    /** 검색 */
    bool Contains(const ElementType& Item) const
    {
        return this->find(Item) != this->end();
    }

    /** 집합 연산 */
    TSet<ElementType> Union(const TSet<ElementType>& Other) const
    {
        TSet<ElementType> Result = *this;
        for (const auto& Item : Other)
        {
            Result.Add(Item);
        }
        return Result;
    }

    TSet<ElementType> Intersect(const TSet<ElementType>& Other) const
    {
        TSet<ElementType> Result;
        for (const auto& Item : *this)
        {
            if (Other.Contains(Item))
            {
                Result.Add(Item);
            }
        }
        return Result;
    }

    TSet<ElementType> Difference(const TSet<ElementType>& Other) const
    {
        TSet<ElementType> Result;
        for (const auto& Item : *this)
        {
            if (!Other.Contains(Item))
            {
                Result.Add(Item);
            }
        }
        return Result;
    }

    /** 배열로 변환 */
    TArray<ElementType> Array() const
    {
        TArray<ElementType> Result;
        Result.Reserve(this->size());
        for (const auto& Item : *this)
        {
            Result.Add(Item);
        }
        return Result;
    }
};

/** TMap - 해시 기반 연관 컨테이너 */
template<typename KeyType, typename ValueType, typename Hasher = std::hash<KeyType>, typename Equal = std::equal_to<KeyType>, typename Alloc = std::allocator<std::pair<const KeyType, ValueType>>>
class TMap : public std::unordered_map<KeyType, ValueType, Hasher, Equal, Alloc>{
public:
	using std::unordered_map<KeyType, ValueType, Hasher, Equal, Alloc>::unordered_map;

    /** 요소 추가/수정 */
    void Add(const KeyType& Key, const ValueType& Value)
    {
        (*this)[Key] = Value;
    }

    template<typename... Args>
    void Emplace(const KeyType& Key, Args&&... args)
    {
        this->emplace(Key, ValueType(std::forward<Args>(args)...));
    }

    /** 제거 */
    bool Remove(const KeyType& Key)
    {
        return this->erase(Key) > 0;
    }

    /** 크기 관련 */
    int32 Num() const
    {
        return static_cast<int32>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        this->clear();
    }

    /** 검색 */
    bool Contains(const KeyType& Key) const
    {
        return this->find(Key) != this->end();
    }

    ValueType* Find(const KeyType& Key)
    {
        auto it = this->find(Key);
        return (it != this->end()) ? &it->second : nullptr;
    }

    const ValueType* Find(const KeyType& Key) const
    {
        auto it = this->find(Key);
        return (it != this->end()) ? &it->second : nullptr;
    }

    /** 찾거나 기본값 반환 */
    ValueType FindRef(const KeyType& Key) const
    {
        auto it = this->find(Key);
        return (it != this->end()) ? it->second : ValueType{};
    }

    /** 키/값 배열 반환 */
    TArray<KeyType> GetKeys() const
    {
        TArray<KeyType> Keys;
        Keys.Reserve(this->size());
        for (const auto& Pair : *this)
        {
            Keys.Add(Pair.first);
        }
        return Keys;
    }

    TArray<ValueType> GetValues() const
    {
        TArray<ValueType> Values;
        Values.Reserve(this->size());
        for (const auto& Pair : *this)
        {
            Values.Add(Pair.second);
        }
        return Values;
    }
};

/** TLinkedList - 연결 리스트 (잘 사용하지 않음) */
template<typename T, typename Alloc = std::allocator<T>>
using TLinkedList = std::list<T, Alloc>;

/** TDoubleLinkedList - 이중 연결 리스트 */
template<typename T, typename Alloc = std::allocator<T>>
using TDoubleLinkedList = std::list<T, Alloc>;

/** 큐 모드 열거형 */
enum class EQueueMode
{
    Spsc,           /** Single Producer Single Consumer (기본 FIFO) */
    Mpmc,           /** Multiple Producer Multiple Consumer */
    Mpsc,           /** Multiple Producer Single Consumer */
    Spmc,           /** Single Producer Multiple Consumer */
    Priority        /** Priority Queue */
};

/** 기본 비교자 (std::less와 동일) */
template<typename T>
struct TDefaultCompare
{
    bool operator()(const T& a, const T& b) const
    {
        return a < b;
    }
};

/** 기본 TQueue - FIFO 큐 */
/** TQueue - FIFO 큐 컨테이너 */
template<typename ElementType, typename Container = std::deque<ElementType>, EQueueMode Mode = EQueueMode::Spsc, typename Compare = TDefaultCompare<ElementType>>
class TQueue : public std::queue<ElementType, Container>
{
public:
    using std::queue<ElementType, Container>::queue;

    /** 요소 추가 */
    void Enqueue(const ElementType& Item)
    {
        this->push(Item);
    }

    /** 요소 제거 */
    bool Dequeue(ElementType& OutItem)
    {
        if (this->empty())
        {
            return false;
        }

        OutItem = this->front();
        this->pop();
        return true;
    }

    /** 맨 앞 요소 확인 */
    bool Peek(ElementType& OutItem) const
    {
        if (this->empty())
        {
            return false;
        }

        OutItem = this->front();
        return true;
    }

    /** 크기 관련 */
    int32 Num() const
    {
        return static_cast<int32>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        *this = TQueue<ElementType, Container, Mode, Compare>();
    }
};

/** Priority Queue를 위한 특수화 - 기본 비교자 */
template<typename ElementType>
class TQueue<ElementType, std::deque<ElementType>, EQueueMode::Priority, TDefaultCompare<ElementType>> : public std::priority_queue<ElementType>
{
public:
    using std::priority_queue<ElementType>::priority_queue;

    void Enqueue(const ElementType& Item)
    {
        this->push(Item);
    }

    bool Dequeue(ElementType& OutItem)
    {
        if (this->empty())
        {
            return false;
        }

        OutItem = this->top();
        this->pop();
        return true;
    }

    bool Peek(ElementType& OutItem) const
    {
        if (this->empty())
        {
            return false;
        }

        OutItem = this->top();
        return true;
    }

    int32 Num() const
    {
        return static_cast<int32>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        *this = TQueue<ElementType, std::vector<ElementType>, EQueueMode::Priority, TDefaultCompare<ElementType>>();
    }
};

/** Priority Queue를 위한 특수화 - 커스텀 비교자 */
template<typename ElementType, typename Compare>
class TQueue<ElementType, std::deque<ElementType>, EQueueMode::Priority, Compare> : public std::priority_queue<ElementType, std::vector<ElementType>, Compare>
{
public:
    using std::priority_queue<ElementType, std::vector<ElementType>, Compare>::priority_queue;

    void Enqueue(const ElementType& Item)
    {
        this->push(Item);
    }

    bool Dequeue(ElementType& OutItem)
    {
        if (this->empty())
        {
            return false;
        }

        OutItem = this->top();
        this->pop();
        return true;
    }

    bool Peek(ElementType& OutItem) const
    {
        if (this->empty())
        {
            return false;
        }

        OutItem = this->top();
        return true;
    }

    int32 Num() const
    {
        return static_cast<int32>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        *this = TQueue<ElementType, std::vector<ElementType>, EQueueMode::Priority, Compare>();
    }
};

/** 편의성을 위한 매크로들 */
#define TPriorityQueue(ElementType) TQueue<ElementType, std::deque<ElementType>, EQueueMode::Priority>
#define TPriorityQueueWithCompare(ElementType, Compare) TQueue<ElementType, std::deque<ElementType>, EQueueMode::Priority, Compare>
