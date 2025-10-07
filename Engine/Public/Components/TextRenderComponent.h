#pragma once
#include "Components/PrimitiveComponent.h"

class ULevel;

class UTextRenderComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UTextRenderComponent, UPrimitiveComponent)
	UTextRenderComponent();
	~UTextRenderComponent();

	void SetInstanceData(const FWstring& Characters);
	void SetText(const FWstring& InText);

	FWstring GetText() const { return Text; }
	ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
	uint32 GetVertexNum() const { return VertexNum; }

	void AddToRenderList(ULevel* Level) override;
	bool IsRayCollided(const FRay& WorldRay, float& Distance) const override { return false; }
	FAABB GetWorldBounds() const override { return FAABB(); }

private:
	FWstring Text;
	ID3D11Buffer* VertexBuffer = nullptr;
	uint32 VertexNum = 0;
};

