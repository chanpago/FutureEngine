// 1.0.0 엔진 문제점 25.10.07
pie모드 중단 시, 메모리 해제 안됨
다중퓨보트 미구현
렌더링 하는데 프러스텀 컬링 안되어있음

-뷰포트 크기 관련
원도우 초기 크기는 1600x900이지만 기본적인 창세팅 때문에 값이 줄어든다
1차(디폴트) : 1600x900→1584x861
2차 : 메뉴바(20) + 탭스트립(36) + 탭바(35) 만큼줄어듦. **최종 크기 FRect(0,91,1584,770)**
**UViewportManager.cpp line36**에서 확인가능 앞으로는 **최종 크기 FRect(0,91,1584,770)**에서 그릴것 하지만 오른쪽에 imgui아웃라이너, 디테일 고정해놓은다고 했으므로 x값은 동적으로 변할 것.














[문제 요약]
- 현재 솔루션은 Visual Studio C++(v143, Windows 10 SDK) 기반의 x64 구성에서 PCH(pch.h)를 사용합니다.
- 코드와 프로젝트 설정을 점검한 결과, 컴파일 단계 오류의 직접 원인은 pch.h에 필요한 표준 헤더가 누락된 점입니다.
- 빌드가 진행되더라도 DirectXTK 링크 경로 설정이 잘못되어 링크 오류로 이어질 가능성이 높습니다.

[컴파일 오류의 직접 원인]
1) pch.h에 표준 헤더 누락으로 인한 심볼 미정의
   - 파일: Engine/pch.h
   - 아래 using 선언들이 존재합니다.
       using std::setw;
       using std::shared_ptr;
       using std::unique_ptr;
   - 그러나 pch.h에는 <iomanip>과 <memory>가 포함되어 있지 않습니다.
   - 결과: MSVC에서 "std에 'setw'가 없습니다" 또는 "std에 'shared_ptr/unique_ptr'가 없습니다" 류의 컴파일 오류가 발생합니다.
   - 수정 방법:
       #include <memory>
       #include <iomanip>
     를 Engine/pch.h의 표준 라이브러리 include 구역에 추가합니다.

[링크 단계에서 추가로 발생 가능한 오류]
2) DirectXTK 라이브러리 경로 설정 불일치
   - pch.h에는 다음과 같은 라이브러리 지정이 있습니다.
       #pragma comment(lib, "DirectXTK/Bin/Debug/DirectXTK.lib")
       #pragma comment(lib, "DirectXTK/Bin/Release/DirectXTK.lib")
   - 실제 라이브러리의 위치는 저장소 내 다음 경로입니다.
       External/Include/DirectXTK/Bin/Debug/DirectXTK.lib
       External/Include/DirectXTK/Bin/Release/DirectXTK.lib
   - 또한 vcxproj의 Linker > AdditionalLibraryDirectories는
       $(SolutionDir)External\Include
     로만 설정되어 있어, 위 상대 경로와 일치하지 않습니다.
   - 결과: 컴파일 후 링크 단계에서 LNK1104(DirectXTK.lib을 찾을 수 없음) 등이 발생할 수 있습니다.
   - 권장 수정:
     - 방법 A(프로젝트 설정 권장):
       Linker > AdditionalLibraryDirectories에 다음 경로를 추가합니다.
         $(SolutionDir)External\Include\DirectXTK\Bin\$(Configuration)
       Linker > AdditionalDependencies에 다음을 추가합니다.
         DirectXTK.lib
       그리고 pch.h의 #pragma comment(lib, ...)는 제거(또는 단순화)합니다.
     - 방법 B(pragma 유지 시):
       pragma의 경로를 실제 경로에 맞추어 수정합니다.
         #pragma comment(lib, "External/Include/DirectXTK/Bin/Debug/DirectXTK.lib")
         #pragma comment(lib, "External/Include/DirectXTK/Bin/Release/DirectXTK.lib")
       (단, 구성 분기에 따라 올바른 경로를 자동으로 선택하도록 관리해야 합니다.)

[그 외 확인 사항(직접 원인은 아니나 혼동 요소)]
- Engine.vcxproj에 ClInclude로 Public\\DirectXTK\\*.h가 나열되어 있으나, 실제 해당 폴더는 존재하지 않습니다. 현재 코드에서 이 헤더들을 include하지 않으므로 컴파일에는 영향이 없지만, 프로젝트 정리 차원에서 제거를 권장합니다.
- PCH 사용: Debug|x64 및 Release|x64에서 PrecompiledHeader=Use로 설정되어 있으며, Engine/pch.cpp가 Create로 설정되어 있습니다. 각 .cpp의 첫 include가 "pch.h"여야 하며, 대부분의 파일에서 준수되고 있음을 확인했습니다. 만약 특정 .cpp에서 누락되면 C1010 류의 PCH 관련 컴파일 오류가 발생합니다.

[요약]
- 즉시 수정해야 하는 컴파일 오류: Engine/pch.h에 <memory>, <iomanip> 추가.
- 이어서 발생할 수 있는 링크 오류: DirectXTK 라이브러리 경로(pragma, AdditionalLibraryDirectories/Dependencies)를 실제 위치에 맞게 수정.

[참고 경로]
- pch.h: Engine/pch.h
- DirectXTK 헤더: External/Include/DirectXTK/Inc/*.h
- DirectXTK 라이브러리: External/Include/DirectXTK/Bin/(Debug|Release)/DirectXTK.lib