#pragma once
#include "Core/Object.h"
#include "Editor/EditorPrimitive.h"
#include "Global/CoreTypes.h"

class ULineBatchRenderer;

class UAxis : public UObject
{
	DECLARE_CLASS(UAxis, UObject)

public:
	UAxis();
	~UAxis() override;

	// /** 기존 개별 렌더링 */
	// void Render();

	/** 배칭 렌더링 */
	void AddToLineBatch(ULineBatchRenderer& LineBatch);

private:
	FEditorPrimitive Primitive;
	TArray<FVertex> AxisVertices;
};
