#include "pch.h"
#include "Components/TextRenderComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Render/Renderer/Renderer.h"
#include "Level/Level.h"
#include <string>

IMPLEMENT_CLASS(UTextRenderComponent, UPrimitiveComponent)

UTextRenderComponent::UTextRenderComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	ComponentType = EComponentType::Text;
	SetVisibility(true);

	//SetText(L"[크래프톤정글게임테크랩] UID:" + std::to_wstring(GetUUID()));
}

UTextRenderComponent::~UTextRenderComponent()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
	}
}

void UTextRenderComponent::SetInstanceData(const FWstring& Characters)
{
	URenderer& Renderer = URenderer::GetInstance();
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	int NumCharacters = Characters.size();
	TArray<FTextVertex> VertexData;
	TArray<FTextVertex> VerticesText =
	{
		// { Position },       U,      V
		{ {0.0f, -0.5f,  0.5f}, {0.0f,   0.0f} }, // 좌상단
		{ {0.0f,  0.5f, -0.5f}, {1.0f,   1.0f} }, // 우하단
		{ {0.0f, -0.5f, -0.5f}, {0.0f,   1.0f} }, // 좌하단

		{ {0.0f,  0.5f, -0.5f}, {1.0f,   1.0f} }, // 우하단
		{ {0.0f, -0.5f,  0.5f}, {0.0f,   0.0f} }, // 좌상단
		{ {0.0f,  0.5f,  0.5f}, {1.0f,   0.0f} }, // 우상단
	};
	for (int Index = 0; Index < NumCharacters; Index++)
	{
		
		float OffsetY = (Index - NumCharacters/2);
		VerticesText[0].Position.Y = OffsetY;
		VerticesText[1].Position.Y = OffsetY+1;
		VerticesText[2].Position.Y = OffsetY;
		VerticesText[3].Position.Y = OffsetY+1;
		VerticesText[4].Position.Y = OffsetY;
		VerticesText[5].Position.Y = OffsetY+1;
		FVector4 Color(1, 1, 1, 1);
		FVector Offset(0.0f, OffsetY, 0.0f);
		uint32 CharIdx = ResourceManager.GetCharInfoIdx(Characters[Index]);
		for (int PIndex = 0; PIndex < 6; PIndex++)
		{
			VerticesText[PIndex].CharId = CharIdx;
		}
		VertexData.Append(VerticesText);
	}
	VertexBuffer = Renderer.CreateVertexBuffer(VertexData);
	VertexNum = static_cast<uint32>(VertexData.size());
}

void UTextRenderComponent::SetText(const FWstring& InText)
{
	Text = InText;
	SetInstanceData(Text);
}

void UTextRenderComponent::AddToRenderList(ULevel* Level)
{
	Level->AddTextComponentToRender(this);
}
