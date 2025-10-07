����: "ULevelManager::ULevelManager: private ����� �������� �� �����ϴ�" ������ ���� �߻�

���� ���
- `ULevelManager`�� �̱��� ��ũ��(`DECLARE_SINGLETON`)�� ����Ͽ� �⺻ ������/�Ҹ��ڸ� `private`�� �����մϴ�.
  - ���� ��ġ: `Public\Manager\Level\LevelManager.h` (���� 11 �α�) �� `DECLARE_SINGLETON(ULevelManager)`
  - ��ũ�� ����: `Global\Macro.h`
    - `DECLARE_SINGLETON(ClassName)`�� ������ �����մϴ�:
      - `public: static ClassName& GetInstance();`
      - `private: ClassName(); virtual ~ClassName();` (����/�̵��� ����)
- �׷��� `DuplicateWorldForPIE`���� �� ���带 ������� ���� �ڵ尡 ����˴ϴ�.
  - ��ġ: `Private\Manager\Level\LevelManager.cpp` (�� ���� 369)
    - `ULevelManager* Dst = NewObject<ULevelManager>();`
  - `NewObject<T>()`�� ���� �Լ� ���ø�(`Public\Core\Object.h`)��, ���ο��� `new T()`�� ȣ���մϴ�.
  - ��������� `new ULevelManager()`�� ȣ��Ǹ�, �̴� `ULevelManager`�� `private` �����ڿ� ���� �ܺ� ������ �Ǿ� �����Ϸ��� ���� ���� ������ �����մϴ�.

���� ����/�ڵ� ���
- ���� Ʈ����: `Private\Manager\Level\LevelManager.cpp` �� `NewObject<ULevelManager>()`
- �̱��� ��ũ�� ����: `Public\Manager\Level\LevelManager.h` �� `DECLARE_SINGLETON(ULevelManager)`
- �̱��� ��ũ�� ����: `Global\Macro.h`
- `NewObject` ����: `Public\Core\Object.h` (���� �Լ� ���ø�, `new T()` ȣ��)

�ΰ� ����
- `IMPLEMENT_SINGLETON(ULevelManager)`�� `GetInstance()` ���ο��� ���� ���� ��ü `static ULevelManager Instance;`�� �����ϹǷ�, Ŭ���� ���� �ƶ����� `private` �����ڿ� ���� �����մϴ�. ���� �̱��� ��ü�� �����Դϴ�.
- ������ ���� �ν��Ͻ��� �����Ϸ��� �õ�(`NewObject<ULevelManager>()`)�� �̱��� ����(������ `private`)�� �浹�� ���Դϴ�.

�ذ� ����(��1)
- �̱��� ��å ����:
  - `DuplicateWorldForPIE`���� ���ο� `ULevelManager`�� ���� ������ ����, ���� ��� �����͸� ���� �����̳�(��: `FWorldState`)�� ����ų�, PIE ���� ���¸� �и�.
- ���� ���� ���:
  - `ULevelManager` ���ο� ���丮(���� ��� �Լ�) �߰�: `static ULevelManager* CreateForPIE();` �� ���ο��� `return new ULevelManager();` (Ŭ���� �����̹Ƿ� private ������ ���� ����).
  - �Ǵ� `template<typename T> friend T* NewObject();`�� `ULevelManager`�� ������ `NewObject`�� ����(friend) �ο�. (��� `NewObject<T>`�� ���� �������� ������ �дٴ� �� ����.)
  - �Ǵ� `DECLARE_SINGLETON`�� `UObject` �Ļ��� ���� �������� ����(�ּ� ó���� `IMPLEMENT_UOBJECT_SINGLETON` ����)�� ���� ��θ� `NewObject` ������� �Ͽ�ȭ.