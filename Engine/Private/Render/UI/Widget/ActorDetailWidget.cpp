#include "pch.h"
#include "Render/UI/Widget/ActorDetailWidget.h"
#include "Render/UI/Widget/TargetComponentWidget.h"
#include "Components/SceneComponent.h"

#include "Level/Level.h"
#include "Manager/Level/World.h"
#include "Actor/Actor.h"
#include "Manager/Ui/UIManager.h"

UActorDetailWidget::UActorDetailWidget()
	:ComponentWidget{ new UTargetComponentWidget() }
	,UIManager { UUIManager::GetInstance()}
{

}

UActorDetailWidget::~UActorDetailWidget()
{
	delete ComponentWidget;
}

void UActorDetailWidget::Initialize()
{
	UE_LOG("ActorDetailWidget: Successfully Initialized");
}

void UActorDetailWidget::PostProcess()
{
	ComponentWidget->ApplyTransformToComponent(UIManager.GetSelectedComponent());
}

void UActorDetailWidget::Update()
{
}
// 위젯을 출력한다.
void UActorDetailWidget::RenderWidget()
{
	//렌더위젯에서 한번 SelectedActor를 얻어와서 모두 인자로 넘김. 문제가 생기면 아래의 함수에서 잘못된 엑터 정보를 읽어온 것
	AActor* SelectedActor = UIManager.GetSelectedActor();
	if (SelectedActor)
	{
	
		RenderActorInfo(SelectedActor);

		RenderNameField();
	}
	else
	{
		ImGui::Separator();
		ImGui::TextUnformatted("No Actor Selected");
	}

	if (bNameChanged && SelectedActor)
	{
		SelectedActor->SetName(FString(ActorNameBuffer));
		bNameChanged = false;
	}
}

// 액터의 정보들을 출력한다.
void UActorDetailWidget::RenderActorInfo(AActor* SelectedActor)
{
	//UActorComponent는 씬의 계층 구조에 포함되지 않음(위치가 없으므로)
	//그래도 리스트에 출력은 해야하지만 아직 SceneComponent아닌 Component가 없으므로 SceneComponent를 가정함.
	USceneComponent* SelectedComponent = UIManager.GetSelectedComponent();

	//ImGui::ShowDemoWindow();
	bool bAlwaysOpen = true;


	ImGui::Text("Actor Details");
	ImGui::SameLine();

	//리플렉션이 없어서 일단 하드코딩함
	const char* ComponentNames[3] = { "Static Mesh Component", "Text Render Component", "Billboard Component" };
	if (ImGui::Button("New Component"))
	{
		ImGui::OpenPopup("ComponentList Popup");
	}
	if (ImGui::Button("Delete Component") && SelectedComponent)
	{
		// 선택된 엑터의 루트 컴포넌트가 아닐때에만 컴포넌트를 삭제합니다.
		if (SelectedComponent != SelectedActor->GetRootComponent())
		{
			SelectedActor->DestroyComponent(SelectedComponent);
			SelectedComponent = nullptr;
		}
	}
	int ComponentIndex = -1;
	if (ImGui::BeginPopup("ComponentList Popup"))
	{
		for (int Index = 0;Index<IM_ARRAYSIZE(ComponentNames) ; Index++)
		{
			if (ImGui::Selectable(ComponentNames[Index]))
			{
				ComponentIndex = Index;
			}
		}
		ImGui::EndPopup();
	}
	if (ComponentIndex > -1)
	{
		switch (ComponentIndex)
		{
		case 0:
			SelectedActor->AddActorComponent(EComponentType::StaticMesh, SelectedComponent);
			break;
		case 1:
			SelectedActor->AddActorComponent(EComponentType::Text, SelectedComponent);
			break;
		case 2:
			SelectedActor->AddActorComponent(EComponentType::Billboard, SelectedComponent);
			break;
		}
	}
	
	ImGui::Separator();
	ImGui::Separator();
	RenderComponentTree(SelectedActor->GetRootComponent(), &SelectedComponent);
	UIManager.SetSelectedComponent(SelectedComponent);

	ImGui::Separator();
	ImGui::Separator();


	ComponentWidget->RenderComponentWidget(SelectedComponent);


	// Actor로만 사용
	/*UClass* ActorClass = Actor->GetClass();
	FString ClassName = ActorClass ? ActorClass->GetName() : "Unknown";*/
	/*ImGui::Text("Class: %s", ClassName.c_str());

	void* ActorPtr = static_cast<void*>(Actor);
	ImGui::Text("Address: %p", ActorPtr);

	uint64 MemoryUsage = Actor->GetAllocatedBytes();
	ImGui::Text("Memory: %llu bytes", MemoryUsage);*/
}

void UActorDetailWidget::RenderComponentTree(USceneComponent* CurrentComponent, USceneComponent** SelectedComponent)
{
	if (!CurrentComponent)
		return;
	//원래 ActorComponent를 다 그려야하는데 아직 SceneComponent가 아니고 actorComponent인 Compoennt가 없어서 이렇게 처리함
	bool bOpen = false;
	const TArray<USceneComponent*>& ChildComponentList = CurrentComponent->GetChildComponents();
	//Arrow를 눌렀을때만 Open
	ImGuiTreeNodeFlags Flag = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	//ImGui CollapsingHeader가 기본적으로 선택되었을때 색상과 같고 해제가 안되서 선택 될때마다
	//컬러를 push해주고 pop해줘야함. 2번 테스트 하는게 좀 비효율적으로 보이지만 다른 방법을 찾지 못했음
	if (*SelectedComponent == CurrentComponent)
	{
		Flag |= ImGuiTreeNodeFlags_Selected;
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
	}
	if (ChildComponentList.Num() == 0)
	{
		Flag |= ImGuiTreeNodeFlags_Leaf;
	}
	bOpen = ImGui::TreeNodeEx(CurrentComponent->GetName().c_str(), Flag);
	 
	if (*SelectedComponent == CurrentComponent)
	{
		ImGui::PopStyleColor();
	}
	//이전 아이템(CollapsingHeader)이 선택되면 true 리턴
	if (ImGui::IsItemClicked())
	{
		*SelectedComponent = CurrentComponent;
	}

	//헤더를 열었을 경우에만 다음 헤더로 출력될 자식 컴포넌트를 스택에 추가함
	if (bOpen)
	{
		for (USceneComponent* ChildComponent : ChildComponentList)	
		{
			RenderComponentTree(ChildComponent, SelectedComponent);
		}
		ImGui::TreePop();
	}

}


void UActorDetailWidget::RenderNameField()
{
	ImGui::Text("Name");
	if (ImGui::InputText("##ActorName", ActorNameBuffer, sizeof(ActorNameBuffer)))
	{
		bNameChanged = true;
	}
}

