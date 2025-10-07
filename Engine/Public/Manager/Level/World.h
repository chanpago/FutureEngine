#pragma once
#include "Core/Object.h" 

class ULevel;
struct FLevelMetadata;
class UEditor;

class UWorld;
extern UWorld* GWorld;

class UWorld : public UObject
{ 

	DECLARE_CLASS(UWorld, UObject)
public: 
	UWorld();   
	UWorld(EWorldType Type);
	virtual ~UWorld() override;

	void Init(EWorldType Type = EWorldType::Editor);
	//void Update() const;
	void Tick();

	// Save & Load System
	bool CreateNewLevel(const FString& InLevelName);
	bool SaveCurrentLevel(const FString& InFilePath) const;
	bool LoadLevel(const FString& InLevelName, const FString& InFilePath);

	void ClearAllLevels();
	void Release();

	// Getter
	ULevel* GetCurrentLevel() const
	{
		return CurrentLevel;
	}
	void SetCurrentLevel(ULevel* NewLevel)
	{
		CurrentLevel = NewLevel;
	}
	UEditor* GetOwningEditor() const;
	void SetOwningEditor(UEditor* InEditor);

	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	// Metadata Conversion Functions
	static FLevelMetadata ConvertLevelToMetadata(ULevel* InLevel);
	static bool LoadLevelFromMetadata(ULevel* InLevel, const FLevelMetadata& InMetadata);

	// Level
	EWorldType GetWorldType() const { return WorldType; }
	void SetWorldType(EWorldType TypeOfWorld) { WorldType = TypeOfWorld; }

	static UWorld* DuplicateWorldForPIE(UWorld* EditorWorld);

	void InitializeActorsForPlay();
	void CleanUpWorld();

	bool IsPIEWorld() { return (WorldType == EWorldType::PIE); }

private:
	UEditor* OwningEditor = nullptr;
	ULevel* CurrentLevel = nullptr; 
	EWorldType WorldType;
};

