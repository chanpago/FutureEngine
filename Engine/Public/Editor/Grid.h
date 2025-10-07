#pragma once
#include "Core/Object.h"
#include "Global/CoreTypes.h"
#include "Editor/EditorPrimitive.h"

class ULineBatchRenderer;

class UGrid : public UObject
{
	DECLARE_CLASS(UGrid, UObject)

public:
	UGrid();
	~UGrid() override;
	void SetLineVertices();
	void SetCellSize(float InCellSize);
	float GetCellSize() const { return CellSize; }

	// /** 기존 개별 렌더링 */
	// void RenderGrid();

	/** 배칭 렌더링 */
	void AddToLineBatch(ULineBatchRenderer& LineBatch);

private:
	void SetupGrid();
	void SaveGridSettings() const;
	void LoadGridSettings();

	float CellSize = 1.0f;
	int32 NumLines = 250;
	FEditorPrimitive Primitive;
	TArray<FVertex> LineVertices;

	static constexpr float DEFAULT_CELL_SIZE = 1.0f;
};
