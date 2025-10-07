#include "pch.h"
#include "Editor/Grid.h"
#include "Render/Renderer/Renderer.h"
#include "Render/Renderer/LineBatchRenderer.h"
#include "Editor/EditorPrimitive.h"
#include "Manager/Path/PathManager.h"

IMPLEMENT_CLASS(UGrid, UObject)

UGrid::UGrid()
{
    Primitive.Color = FVector4(0.2f, 0.2f, 0.2f, 0.2f);
	LoadGridSettings();
	SetupGrid();
}

UGrid::~UGrid()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
}

// void UGrid::RenderGrid()
// {
// 	URenderer& Renderer = URenderer::GetInstance();
//
// 	if (Renderer.IsShowFlagEnabled(EEngineShowFlags::SF_Grid) == false)
// 	{
// 		return;
// 	}
//
// 	Renderer.RenderEditorPrimitive(Primitive, Primitive.RenderState);
// }

void UGrid::AddToLineBatch(ULineBatchRenderer& LineBatch)
{
	URenderer& Renderer = URenderer::GetInstance();

	if (Renderer.IsShowFlagEnabled(EEngineShowFlags::SF_Grid) == false)
	{
		return;
	}

	/** 기존 LineVertices 데이터를 배칭에 추가 */
	LineBatch.AddLines(LineVertices);
}

void UGrid::SetupGrid()
{
	URenderer& Renderer = URenderer::GetInstance();
	if (Primitive.Vertexbuffer)
	{
		Renderer.ReleaseVertexBuffer(Primitive.Vertexbuffer);
		Primitive.Vertexbuffer = nullptr;
	}

	SetLineVertices();

	Primitive.NumVertices = static_cast<uint32>(LineVertices.size());
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(LineVertices);
	Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);
}

void UGrid::SaveGridSettings() const
{
	const path ConfigFilePath = UPathManager::GetInstance().GetConfigPath() / "editor.ini";

	WritePrivateProfileStringA(
		"Grid",
		"CellSize",
		std::to_string(CellSize).c_str(),
		ConfigFilePath.string().c_str()
	);
}

void UGrid::LoadGridSettings()
{
	const path ConfigFilePath = UPathManager::GetInstance().GetConfigPath() / "editor.ini";

	// Check if config file exists
	if (!std::filesystem::exists(ConfigFilePath))
	{
		// Create default config if it doesn't exist
		SaveGridSettings();
		return;
	}

	char Buffer[32];

	// Load Move Speed
	GetPrivateProfileStringA(
		"Grid",
		"CellSize",
		std::to_string(DEFAULT_CELL_SIZE).c_str(),
		Buffer,
		sizeof(Buffer),
		ConfigFilePath.string().c_str()
	);

	float LoadedCellSize = std::stof(Buffer);
	SetCellSize(LoadedCellSize);
}

void UGrid::SetLineVertices()
{
	LineVertices.clear();
	float LineLength = CellSize * static_cast<float>(NumLines) / 2.f;

	// y축 라인
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount) // z축 라인
	{
		if (LineCount == 0)
		{
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize, -LineLength, 0.f}, Primitive.Color });
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize, 0.f, 0.f}, Primitive.Color });
		}
		else
		{
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize, -LineLength, 0.f}, Primitive.Color });
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize, LineLength, 0.f}, Primitive.Color });
		}
	}

	// x축 라인
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount) // x축 라인
	{
		if (LineCount == 0)
		{
			LineVertices.push_back({ {-LineLength, static_cast<float>(LineCount) * CellSize, 0.f}, Primitive.Color });
			LineVertices.push_back({ {0.f, static_cast<float>(LineCount) * CellSize, 0.f}, Primitive.Color });
		}
		else
		{
			LineVertices.push_back({ {-LineLength, static_cast<float>(LineCount) * CellSize, 0.f}, Primitive.Color });
			LineVertices.push_back({ {LineLength, static_cast<float>(LineCount) * CellSize, 0.f}, Primitive.Color });
		}
	}

}

void UGrid::SetCellSize(float InCellSize)
{
	const float TotalLength = 250.0f;

	CellSize = InCellSize;

	NumLines = static_cast<int>(TotalLength / CellSize);

	// 라인 수가 홀수일 경우 중앙선을 포함하도록 조정
	if (NumLines % 2 != 0)
	{
		NumLines++;
	}
	SaveGridSettings();
	SetupGrid();
}
