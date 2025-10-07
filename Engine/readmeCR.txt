문제: "ULevelManager::ULevelManager: private 멤버에 엑세스할 수 없습니다" 컴파일 오류 발생

원인 요약
- `ULevelManager`는 싱글턴 매크로(`DECLARE_SINGLETON`)를 사용하여 기본 생성자/소멸자를 `private`로 선언합니다.
  - 선언 위치: `Public\Manager\Level\LevelManager.h` (라인 11 부근) → `DECLARE_SINGLETON(ULevelManager)`
  - 매크로 정의: `Global\Macro.h`
    - `DECLARE_SINGLETON(ClassName)`는 다음을 포함합니다:
      - `public: static ClassName& GetInstance();`
      - `private: ClassName(); virtual ~ClassName();` (복사/이동도 삭제)
- 그런데 `DuplicateWorldForPIE`에서 새 월드를 만들려고 다음 코드가 실행됩니다.
  - 위치: `Private\Manager\Level\LevelManager.cpp` (약 라인 369)
    - `ULevelManager* Dst = NewObject<ULevelManager>();`
  - `NewObject<T>()`는 자유 함수 템플릿(`Public\Core\Object.h`)로, 내부에서 `new T()`를 호출합니다.
  - 결과적으로 `new ULevelManager()`가 호출되며, 이는 `ULevelManager`의 `private` 생성자에 대한 외부 접근이 되어 컴파일러가 접근 위반 오류를 보고합니다.

증거 파일/코드 경로
- 오류 트리거: `Private\Manager\Level\LevelManager.cpp` → `NewObject<ULevelManager>()`
- 싱글턴 매크로 선언: `Public\Manager\Level\LevelManager.h` → `DECLARE_SINGLETON(ULevelManager)`
- 싱글턴 매크로 정의: `Global\Macro.h`
- `NewObject` 정의: `Public\Core\Object.h` (자유 함수 템플릿, `new T()` 호출)

부가 설명
- `IMPLEMENT_SINGLETON(ULevelManager)`는 `GetInstance()` 내부에서 정적 지역 객체 `static ULevelManager Instance;`를 생성하므로, 클래스 내부 맥락에서 `private` 생성자에 접근 가능합니다. 따라서 싱글턴 자체는 정상입니다.
- 문제는 별도 인스턴스를 생성하려는 시도(`NewObject<ULevelManager>()`)가 싱글턴 제약(생성자 `private`)과 충돌한 것입니다.

해결 방향(택1)
- 싱글턴 정책 유지:
  - `DuplicateWorldForPIE`에서 새로운 `ULevelManager`를 직접 만들지 말고, 복제 대상 데이터를 별도 컨테이너(예: `FWorldState`)로 만들거나, PIE 전용 상태만 분리.
- 별도 생성 허용:
  - `ULevelManager` 내부에 팩토리(정적 멤버 함수) 추가: `static ULevelManager* CreateForPIE();` → 내부에서 `return new ULevelManager();` (클래스 내부이므로 private 생성자 접근 가능).
  - 또는 `template<typename T> friend T* NewObject();`를 `ULevelManager`에 선언해 `NewObject`에 우정(friend) 부여. (모든 `NewObject<T>`가 접근 가능해져 범위가 넓다는 점 주의.)
  - 또는 `DECLARE_SINGLETON`을 `UObject` 파생형 전용 버전으로 수정(주석 처리된 `IMPLEMENT_UOBJECT_SINGLETON` 참고)해 생성 경로를 `NewObject` 기반으로 일원화.