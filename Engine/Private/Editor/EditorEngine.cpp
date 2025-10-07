#include "pch.h"

#include "Editor/EditorEngine.h"
#include "Manager/Level/World.h"
#include "Level/Level.h"
#include "Actor/Actor.h"

IMPLEMENT_CLASS(UEditorEngine, UObject)

UEditorEngine* GEditor = nullptr;

bool InitEditorEngine()
{
	if (GEditor != nullptr) return false;

	GEditor = NewObject<UEditorEngine>();
	//GEditor = new UEditorEngine(); 
	GWorld->Init();
	return true;
}

void ShutdownEditorEngine()
{
	if (!GEditor) return;

	delete GEditor;
	GEditor = nullptr; 
}

void UEditorEngine::Tick()
{
	GWorld->Tick();
}

FWorldContext UEditorEngine::GetEditorWorldContext()
{
	for (FWorldContext& WorldContext : WorldContexts)
	{
		if (WorldContext.WorldType == EWorldType::Editor)
		{
			return WorldContext;
		}
	}
}

FWorldContext UEditorEngine::GetPIEWorldContext()
{
	for (FWorldContext& WorldContext : WorldContexts)
	{
		if (WorldContext.WorldType == EWorldType::PIE)
		{
			return WorldContext;
		}
	}
}

void UEditorEngine::AddWorldContext(FWorldContext WorldContext)
{
	if (GEditor == nullptr)
	{
		InitEditorEngine();
	}
	WorldContexts.emplace_back(WorldContext);
}

bool UEditorEngine::RemoveWorldContext(EWorldType WorldType)
{
	for (const FWorldContext& WorldContext : WorldContexts)
	{
		if (WorldContext.WorldType == WorldType)
		{
			WorldContexts.Remove(WorldContext);
	
			return true;
		}
	}

	return false;
}

