#include "pch.h"
#include "Core/ObjectIterator.h"


// 자주 쓰는 타입을 명시적 인스턴스화해서 링크 타임/빌드 비용을 약간 줄일 수 있음.
#include "Mesh/StaticMesh.h"     // UStaticMesh
//#include "Components/SceneComponent.h"     // USceneComponent
//#include "Rendering/Material.h"     // UMaterial (필요시)
//#include "Light/LightComponent.h"   // ULightComponent (필요시)

template class FObjectIterator<UStaticMesh>;
//template class FObjectIterator<USceneComponent>;
//template class FObjectIterator<UMaterial>;
//template class FObjectIterator<ULightComponent>;
