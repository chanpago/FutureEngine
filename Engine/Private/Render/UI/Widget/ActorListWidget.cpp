#include "pch.h"
#include "Render/UI/Widget/ActorListWidget.h"

#include "Level/Level.h"
#include "Manager/Level/World.h"
#include "Actor/Actor.h"
#include "Manager/UI/UIManager.h"
#include "Actor/Actor.h"

UActorListWidget::UActorListWidget()
{
}

void UActorListWidget::Initialize()
{
	UE_LOG("ActorListWidget: Successfully Initialized");
}

void UActorListWidget::Update()
{
	ULevel* Level = GWorld->GetCurrentLevel();
	CurrentLevel = Level;

	if (!Level)
	{
		SelectedActor = nullptr;
		return;
	}

	AActor* CurrSel = UUIManager::GetInstance().GetSelectedActor();
	if (!Level->IsActorValid(CurrSel)) CurrSel = nullptr;

	if (SelectedActor != CurrSel)
	{
		SelectedActor = CurrSel;
	}
}

void UActorListWidget::RenderWidget()
{
	ImGui::Text("Scene Actors");
	ImGui::SameLine();
	if (ImGui::Button("New Actor"))
	{
		GWorld->GetCurrentLevel()->SpawnActor<AActor>("Actor");
		
	}
	ImGui::Separator();

	if (CurrentLevel)
	{
		RenderActorList();
	}
	else
	{
		ImGui::TextUnformatted("No Level Loaded");
	}
}

void UActorListWidget::RenderActorList()
{
	const TArray<AActor*>& Actors = CurrentLevel->GetLevelActors();

	if (Actors.empty())
	{
		ImGui::TextUnformatted("No Actors in Scene");
		return;
	}

	if (ImGui::BeginChild("ActorList", ImVec2(0, 0), true))
	{
		for (int32 i = 0; i < Actors.size(); ++i)
		{
			if (Actors[i])
			{
				RenderActorItem(Actors[i], i);
			}
		}
	}
	ImGui::EndChild();
}

void UActorListWidget::RenderActorItem(AActor* InActor, int32 InIndex)
{
	FString DisplayName = GetActorDisplayName(InActor);
	FString ClassName = GetActorClassName(InActor);

	bool bIsSelected = (InActor == SelectedActor);

	if (ImGui::Selectable(DisplayName.c_str(), bIsSelected))
	{
		if (CurrentLevel)
		{
			UUIManager::GetInstance().SetSelectedActor(InActor);
			SelectedActorIndex = InIndex;
		}
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("Name: %s", DisplayName.c_str());
		ImGui::Text("Class: %s", ClassName.c_str());
		ImGui::EndTooltip();
	}
}

FString UActorListWidget::GetActorDisplayName(AActor* InActor) const
{
	if (!InActor)
		return "None";

	FString ActorName = InActor->GetName();
	if (ActorName.empty())
	{
		return GetActorClassName(InActor) + "_" + to_string(reinterpret_cast<uintptr_t>(InActor));
	}

	return ActorName;
}

FString UActorListWidget::GetActorClassName(AActor* InActor) const
{
	if (!InActor)
		return "Unknown";

	UClass* ActorClass = InActor->GetClass();
	if (ActorClass)
	{
		return ActorClass->GetName();
	}

	return "Unknown";
}
