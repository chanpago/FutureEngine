목적
- 최소 수정으로 Editor 모드의 물체가 PIE(WorldType: PIE) 세계에 그대로 나타나고, PIE에서의 변경이 Editor 모드에 영향을 주지 않도록 Duplicate 기반 구조로 분리했습니다.

핵심 변경 요약
- 깊은 복제 경로 구현
  - AActor::Duplicate / DuplicateSubObjects: 루트/소유 컴포넌트를 새 인스턴스로 복제하고, 새 Actor에 Outer/Owner를 재설정하여 독립시킴.
  - UActorComponent, USceneComponent, UPrimitiveComponent, UStaticMeshComponent의 Duplicate/CopyShallow를 실제 복제 동작하도록 구현.
    - Scene: 위치/회전(Quat 동기)/스케일 등 트랜스폼 복사.
    - Primitive: 가시성, 색상 등 렌더 상태 복사, 옥트리 인덱스 초기화.
    - StaticMesh: 월드 경계(WorldBound), StaticMesh 포인터, 머티리얼 리스트 복사.
- Level 등록 훅 추가
  - ULevel::AddActor(AActor*): 이미 생성된 Actor를 레벨에 등록(Outer/Level 지정, LevelActors에 추가, Primitive 루트가 있으면 옥트리에 등록).
- PIE 월드 복제 경로 수정
  - UWorld::DuplicateWorldForPIE(EditorWorld): Editor의 현재 Level에서 Actor들을 Duplicate하여 새 PIE Level에 AddActor로 등록. GWorld 루프 대신 SrcLevel을 직접 사용.
  - PIE 월드 타입 설정 및 레벨 세팅 유지.
- 기타 정리
  - AActor::Duplicate에서 Editor Outer 상속 제거(PIE에서 레벨이 재지정).
  - 컴포넌트 재결합 시 TextComponent를 새 루트에 붙여주는 보정 로직 추가.
  - Actor.cpp에 TextComponent include 추가.

변경 파일 목록
- Private/Actor/Actor.cpp
- Private/Components/ActorComponent.cpp
- Private/Components/SceneComponent.cpp
- Private/Components/PrimitiveComponent.cpp
- Private/Components/StaticMeshComponent.cpp
- Public/Level/Level.h
- Private/Level/Level.cpp
- Private/Manager/Level/World .cpp

동작 방식
- PIE 시작(Play) 시 Editor의 현재 레벨의 모든 Actor를 Duplicate하여 PIE 레벨에 등록합니다.
- 복제된 컴포넌트들은 메쉬/머티리얼/트랜스폼/가시성/색상 등 렌더에 필요한 상태를 복사하므로, Editor에서 보이던 모습이 그대로 PIE에서 재현됩니다.
- PIE 인스턴스는 Editor 인스턴스와 Outer/Owner가 완전히 분리되어 있어, PIE에서의 위치/머티리얼/가시성 등의 변경이 Editor에 영향을 주지 않습니다.

사용 방법
- 상단 PIE UI에서 Play 버튼 클릭 또는 Ctrl+P.
- PIE 중에 오브젝트를 이동/수정해도 Editor에는 적용되지 않습니다.
- Stop을 누르면 PIE World를 정리하고 GWorld를 EditorWorld로 복구합니다.

주의/남은 일
- PIEWidget의 Ctrl+P 토글 분기(Playing/Stopped) 키 처리 로직이 반대로 보일 수 있습니다. 필요 시 정리해드릴 수 있습니다.
- StaticMeshComponent::CopyShallow에서 SetStaticMesh를 호출하여 슬롯 수를 맞춘 뒤 기존 머티리얼을 복사합니다(리소스 공유).
