#pragma once
#include "Core/Object.h"
#include "Global/Types.h"

class UEditorEngine;
extern UEditorEngine* GEditor;

bool InitEditorEngine();
void ShutdownEditorEngine();

class UEditorEngine : public UObject
{
	DECLARE_CLASS(UEditorEngine, UObject)

public:
	 
	virtual void Tick();

	FWorldContext GetEditorWorldContext();
	FWorldContext GetPIEWorldContext();
	void AddWorldContext(FWorldContext WorldContext);
	bool RemoveWorldContext(EWorldType WorldType);

private:
	TArray<FWorldContext> WorldContexts;
};

