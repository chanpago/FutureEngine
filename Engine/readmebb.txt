문제: DuplicateWorldForPIE → AddToOctree 호출 시 AABB(바운딩 박스) 없음으로 크래시

요약
- PIE 시작 시 `UWorld::DuplicateWorldForPIE` 안에서 복제된 액터의 루트 프리미티브 컴포넌트를 `AddToOctree`에 바로 넣으면서 크래시가 발생합니다.
- 원인은 “유효한 AABB가 준비되지 않은 상태”에서 옥트리에 삽입되기 때문입니다. 실제로는 두 가지 근본 문제가 겹쳐 있습니다.
  1) 복제된 컴포넌트(특히 `UStaticMeshComponent`)가 유효한 StaticMesh 없이 `GetWorldBounds()`를 호출하게 됨.
  2) 옥트리 루트 AABB를 사전에 산출하지 않고 기본값(-50~50)으로 두고 요소 삽입을 시도함.

크래시 포인트(경로/코드 흐름)
- `Private/Manager/Level/World .cpp` → `UWorld::DuplicateWorldForPIE`
  - 현재 구현: 액터를 `Actor->Duplicate()`로 복제 → `DstLevel->AddToOctree(Cast<UPrimitiveComponent>(DupActor->GetRootComponent()));`
- `Public/Level/Level.h` / `Private/Level/Level.cpp`
  - `ULevel::AddToOctree()` → `StaticOctree.AddElement(Component, 0);`
- `Public/Math/Octree.h`
  - `TOctree::AddElement()` → `TTrait::GetWorldAABB(Element)` → `UPrimitiveComponent::GetWorldBounds()` 호출
- `Private/Components/StaticMeshComponent.cpp`
  - `UStaticMeshComponent::GetWorldBounds()`는 `StaticMesh->GetPrimitiveType()` 접근(Null 가드 없음) → StaticMesh가 없으면 즉시 크래시

근본 원인 상세
1) 컴포넌트 복제 미구현/불완전
   - `UActorComponent::Duplicate()`가 현재 `nullptr`을 반환(미구현). 이 영향으로 `AActor::DuplicateSubObjects()`에서
     - `RootComponent = RootComponent->Duplicate()`는 루트가 `nullptr`로 바뀔 수 있고,
     - `OwnedComponents`는 로컬 변수 재할당(셋에 반영되지 않음)로 그대로 원본을 들고 있게 됩니다.
   - 결과적으로 PIE 복제 직후 `DupActor->GetRootComponent()`가 `nullptr`이거나, 루트가 `UPrimitiveComponent`가 아니거나,
     또는 `UStaticMeshComponent`에 StaticMesh가 설정되지 않은 상태로 `GetWorldBounds()`가 호출됩니다.

2) 옥트리 루트 AABB 미초기화
   - `LoadLevelFromMetadata()`에서는 모든 프리미티브의 AABB를 먼저 합산해 `OctreeSize`를 만든 뒤 `NewOctree(OctreeSize)`로 초기화하고 삽입합니다.
   - 반면 `DuplicateWorldForPIE()`는 합산 없이 바로 삽입합니다. 바운드가 기본 루트(-50~50)를 벗어나거나, 요소 바운드 자체가 무효하면
     내부 Contains/분기에서 비정상 경로를 타다 충돌 위험이 커집니다.

권장 해결책(안전하고 재현 가능한 순서)
 A. PIE 복제 루틴에서 옥트리 루트부터 준비
   - 복제된 액터들의 프리미티브 컴포넌트를 먼저 한 바퀴 수집하고, 유효한 바운드를 합산해 `OctreeSize`를 만듭니다.
   - 그 다음에 `DstLevel->NewOctree(OctreeSize)`를 호출한 뒤, 컴포넌트를 옥트리에 삽입하세요.

   예시 패턴(의사 코드):
   - 복제 루프 1: 액터 복제/스폰만 수행 → 컴포넌트 수집 → `if (Comp && Comp->GetWorldBounds().IsValid()) OctreeSize.AddAABB(...)`
   - 루트 초기화: `DstLevel->NewOctree(OctreeSize)`
   - 복제 루프 2: `if (Comp && Comp->GetWorldBounds().IsValid()) DstLevel->AddToOctree(Comp);`

 B. AddToOctree/Octree 레벨에서 방어 코드 추가
   - `ULevel::AddToOctree(UPrimitiveComponent* Component)`에서 즉시 가드:
     - `if (!Component) return;`
     - `const FAABB b = Component->GetWorldBounds(); if (!b.IsValid()) return;`
   - 또는 `TOctree::AddElement()` 진입부에서 `Bound.IsValid()`가 아니면 삽입을 무시하도록 빠른 리턴을 추가.

 C. StaticMeshComponent의 바운드 요청 시 Null 가드
   - `UStaticMeshComponent::GetWorldBounds()`에 StaticMesh null 가드를 추가하세요.
     - `if (!StaticMesh) return FAABB();`처럼 무효 AABB를 반환하면, 위 B의 방어코드로 안전하게 스킵됩니다.

 D. 컴포넌트 복제 정비(근본 개선)
   - 단기: `DuplicateWorldForPIE()`에서 `AActor::Duplicate()` 대신 이미 있는 안전 경로를 사용
     - 주어진 `AActor::DuplicateForTest(ULevel* NewOuter, AActor* OldActor)`는 스폰 후 트랜스폼/메시를 명시적으로 설정하고
       직후 `NewOuter->AddToOctree(...)`을 호출합니다. 이 함수를 활용하되, 위 A/B/C 가드를 같이 적용하면 안전합니다.
     - 실제 코드에선 주석 처리된 `DuplicateForTest` 호출을 복원하고(또는 동등한 스폰 복제를 새로 작성), 옥트리 루트 산출을 함께 넣으세요.
   - 중기: `UActorComponent::Duplicate()` 구현 또는 `AActor::DuplicateSubObjects()`의 셋 재할당 버그 수정
     - 현재 `UActorComponent::Duplicate()`는 `nullptr`을 반환하므로, 최소한 동일 클래스의 인스턴스를 생성해 필요한 필드(Owner/Outer/트랜스폼/리소스 포인터)를 얕은 복사하는 수준이라도 구현해야 합니다.
     - `AActor::DuplicateSubObjects()`에서 `for (UActorComponent* Comp : OwnedComponents)`는 로컬 복사이므로, 새 컴포넌트를 셋에 반영하도록 변경해야 합니다.

구체적인 패치 가이드(추천 스니펫)
1) ULevel::AddToOctree 가드
   파일: `Private/Level/Level.cpp`
   기존:
     void ULevel::AddToOctree(UPrimitiveComponent* Component)
     {
         StaticOctree.AddElement(Component, 0);
     }
   변경 예:
     void ULevel::AddToOctree(UPrimitiveComponent* Component)
     {
         if (!Component) return;
         const FAABB b = Component->GetWorldBounds();
         if (!b.IsValid()) return;
         StaticOctree.AddElement(Component, 0);
     }

2) UStaticMeshComponent::GetWorldBounds Null 가드
   파일: `Private/Components/StaticMeshComponent.cpp`
   상단에 즉시 추가:
     if (!StaticMesh)
     {
         return FAABB();
     }
   그런 다음 기존 switch문 진행.

3) DuplicateWorldForPIE: 옥트리 루트 산출 + 안전 삽입
   파일: `Private/Manager/Level/World .cpp`
   흐름 예시:
     UWorld* UWorld::DuplicateWorldForPIE(UWorld* EditorWorld)
     {
         UWorld* Dst = NewObject<UWorld>();
         Dst->Init(EWorldType::PIE);
         Dst->SetWorldType(EWorldType::PIE);

         ULevel* SrcLevel = EditorWorld->GetCurrentLevel();
         ULevel* DstLevel = new ULevel(SrcLevel->GetName() + "_PIE");
         Dst->SetCurrentLevel(DstLevel);

         // 1) 액터 스폰/복제만 먼저 수행하고 컴포넌트 수집
         TArray<UPrimitiveComponent*> Comps;
         FAABB OctreeSize;
         for (AActor* Actor : SrcLevel->GetLevelActors())
         {
             // 안전 경로: DuplicateForTest 또는 직접 스폰 + 메시/트랜스폼 복사
             AActor* NewA = /*Spawn & copy*/;
             if (!NewA) continue;
             auto* Prim = Cast<UPrimitiveComponent>(NewA->GetRootComponent());
             if (!Prim) continue;
             const FAABB b = Prim->GetWorldBounds();
             if (b.IsValid()) { OctreeSize.AddAABB(b); Comps.Add(Prim); }
         }

         // 2) 옥트리 루트 초기화
         DstLevel->NewOctree(OctreeSize);

         // 3) 삽입(유효 바운드만)
         for (auto* C : Comps) { DstLevel->AddToOctree(C); }
         return Dst;
     }

4) 임시 조치: DuplicateForTest 사용
   - 현재 파일에 이미 존재: `AActor::DuplicateForTest(ULevel* NewOuter, AActor* OldActor)`
   - `DuplicateWorldForPIE()`에서 `Actor->Duplicate()` 대신 이 함수를 사용하면 StaticMesh/트랜스폼이 명확히 세팅되어 바운드 계산이 쉬워집니다.
   - 단, 여전히 1)과 3)의 옥트리 루트 산출과 AddToOctree 가드는 필요합니다.

검증 절차
1) PIE 실행(Play 버튼 또는 Ctrl+P)
2) 크래시 없이 PIE 월드 생성 및 렌더링 확인
3) Alt+C 등으로 Bounds 디버그를 켜고, 바운드가 정상적으로 보이는지 확인
4) 에디터에 StaticMesh가 비어 있는 액터를 일부 남겨둔 상태에서도(의도적으로) 크래시가 나지 않고, 해당 액터는 옥트리에 삽입이 스킵되는지 확인

부가 참고
- 현재 `AActor::Duplicate()`/`DuplicateSubObjects()`/`UActorComponent::Duplicate()`는 실제 딥카피 로직이 부재 또는 버그가 있습니다.
  - 크래시를 막는 1차 조치는 위의 가드/초기화로 충분하지만, 장기적으로는 컴포넌트 복제 경로를 정비해야 합니다.
- `LoadLevelFromMetadata()`가 보여주는 패턴(먼저 AABB를 합산해 옥트리 루트를 만든 뒤 삽입)은 안전한 초기화 순서의 좋은 예입니다.

요약(Checklist)
- [필수] AddToOctree/Octree에 Null/Invalid AABB 가드 추가
- [필수] DuplicateWorldForPIE에서 옥트리 루트 AABB를 먼저 합산해 `NewOctree()`로 초기화
- [권장] StaticMeshComponent::GetWorldBounds에 StaticMesh null 가드 추가
- [권장] DuplicateForTest로 복제 안전화 또는 컴포넌트 Duplicate 구현 보완

