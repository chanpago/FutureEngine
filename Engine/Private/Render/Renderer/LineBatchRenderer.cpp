#include "pch.h"
#include "Render/Renderer/LineBatchRenderer.h"
#include "Render/Renderer/Renderer.h"
#include "Render/Renderer/Pipeline.h"
#include "Global/Enum.h"

IMPLEMENT_CLASS(ULineBatchRenderer, UObject)
IMPLEMENT_SINGLETON(ULineBatchRenderer)

ULineBatchRenderer::ULineBatchRenderer() = default;
ULineBatchRenderer::~ULineBatchRenderer() = default;

void ULineBatchRenderer::Init()
{
    CreateBuffers();

	/** 배칭 데이터 예약 */
	BatchedVertices.reserve(MaxVertices);
	BatchedIndices.reserve(MaxIndices);

	PipelineDescKeyLine.BlendType = EBlendType::Opaque;
	PipelineDescKeyLine.DepthStencilType = EDepthStencilType::Opaque;
	PipelineDescKeyLine.ShaderType = EShaderType::SampleShader;
	PipelineDescKeyLine.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	FRasterizerKey RasterizerKey;
	RasterizerKey.CullMode = D3D11_CULL_NONE;
	RasterizerKey.FillMode = D3D11_FILL_SOLID;
	PipelineDescKeyLine.RasterizerKey = RasterizerKey;

}

void ULineBatchRenderer::Release()
{
    ReleaseBuffers();
    BatchedVertices.clear();
    BatchedIndices.clear();
}

void ULineBatchRenderer::CreateBuffers()
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();

	/** 동적 Vertex Buffer 생성 */
	{
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.ByteWidth = MaxVertices * sizeof(FVertex);
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		HRESULT HR = Device->CreateBuffer(&BufferDesc, nullptr, &DynamicVertexBuffer);
		if (FAILED(HR))
		{
			UE_LOG("Failed to create dynamic vertex buffer for LineBatchRenderer");
		}
	}

	/** 동적 Index Buffer 생성 */
	{
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.ByteWidth = MaxIndices * sizeof(uint32);
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		HRESULT HR = Device->CreateBuffer(&BufferDesc, nullptr, &DynamicIndexBuffer);
		if (FAILED(HR))
		{
			UE_LOG("Failed to create dynamic index buffer for LineBatchRenderer");
		}
	}
}

void ULineBatchRenderer::ReleaseBuffers()
{
	if (DynamicVertexBuffer)
	{
		DynamicVertexBuffer->Release();
		DynamicVertexBuffer = nullptr;
	}

	if (DynamicIndexBuffer)
	{
		DynamicIndexBuffer->Release();
		DynamicIndexBuffer = nullptr;
	}
}

void ULineBatchRenderer::BeginBatch()
{
	if (bBatchStarted)
	{
		UE_LOG("Warning: BeginBatch called while batch already started");
		return;
	}

    ClearBatch();
    bBatchStarted = true;
}

void ULineBatchRenderer::ClearBatch()
{
    BatchedVertices.clear();
    BatchedIndices.clear();
    CurrentVertexCount = 0;
    CurrentIndexCount = 0;
}

void ULineBatchRenderer::FlushBatch()
{
	if (!bBatchStarted)
	{
		UE_LOG("Warning: FlushBatch called without BeginBatch");
		return;
	}

	if (CurrentVertexCount == 0)
	{
		bBatchStarted = false;
		return;
	}

	/** 버퍼 업데이트 및 렌더링 */
    UpdateVertexBuffer();
    UpdateIndexBuffer();
    RenderBatch();

    // (indexed) line rendering only

	/** 배치 종료 */
	bBatchStarted = false;
}

void ULineBatchRenderer::AddLine(const FVector& Start, const FVector& End, const FVector4& Color)
{
	if (!bBatchStarted)
	{
		UE_LOG("Warning: AddLine called without BeginBatch");
		return;
	}

	if (!CanAddVertices(2))
	{
		UE_LOG("Warning: Line batch is full, cannot add more lines");
		return;
	}

	/** 시작점과 끝점 정점 추가 */
	BatchedVertices.push_back({Start, Color});
	BatchedVertices.push_back({End, Color});

	/** 라인 인덱스 생성 (2개 정점) */
	GenerateLineIndices(CurrentVertexCount, 2);
	CurrentVertexCount += 2;
}

void ULineBatchRenderer::AddLines(const TArray<FVertex>& Vertices)
{
	if (!bBatchStarted)
	{
		UE_LOG("Warning: AddLines called without BeginBatch");
		return;
	}

	uint32 VertexCount = static_cast<uint32>(Vertices.size());
	if (!CanAddVertices(VertexCount))
	{
		UE_LOG("Warning: Cannot add %d vertices to line batch (current: %d, max: %d)",
			VertexCount, CurrentVertexCount, MaxVertices);
		return;
	}

	/** 정점 데이터 추가 */
	for (const FVertex& Vertex : Vertices)
	{
		BatchedVertices.push_back(Vertex);
	}

	/** 라인 인덱스 생성 (정점 2개씩 페어) */
	GenerateLineIndices(CurrentVertexCount, VertexCount);
	CurrentVertexCount += VertexCount;
}

void ULineBatchRenderer::AddIndexedLines(const TArray<FVertex>& Vertices, const TArray<uint32>& Indices)
{
    if (!bBatchStarted)
    {
        UE_LOG("Warning: AddIndexedLines called without BeginBatch");
        return;
    }

    if (Vertices.IsEmpty() || Indices.IsEmpty())
        return;

    if (!CanAddVertices(static_cast<uint32>(Vertices.size())))
    {
        UE_LOG("Warning: Cannot add %d vertices to line batch (current: %d, max: %d)",
            static_cast<uint32>(Vertices.size()), CurrentVertexCount, MaxVertices);
        return;
    }

    // Append vertices
    uint32 base = CurrentVertexCount;
    for (const FVertex& V : Vertices)
    {
        BatchedVertices.push_back(V);
    }
    CurrentVertexCount += static_cast<uint32>(Vertices.size());

    // Append indices (pairs for lines)
    if ((Indices.size() % 2) != 0)
    {
        UE_LOG("Warning: Indices size (%d) is not even for line list", static_cast<uint32>(Indices.size()));
    }
    for (size_t i = 0; i + 1 < Indices.size(); i += 2)
    {
        BatchedIndices.push_back(base + static_cast<uint32>(Indices[i]));
        BatchedIndices.push_back(base + static_cast<uint32>(Indices[i + 1]));
        CurrentIndexCount += 2;
    }
}

void ULineBatchRenderer::AddLineStrip(const TArray<FVertex>& Vertices, bool bClosed)
{
	if (!bBatchStarted)
	{
		UE_LOG("Warning: AddLineStrip called without BeginBatch");
		return;
	}

	uint32 VertexCount = static_cast<uint32>(Vertices.size());
	if (VertexCount < 2)
	{
		/** 최소 2개 정점 필요 */
		return;
	}

	if (!CanAddVertices(VertexCount))
	{
		UE_LOG("Warning: Cannot add %d vertices to line batch", VertexCount);
		return;
	}

	/** 정점 데이터 추가 */
	for (const FVertex& Vertex : Vertices)
	{
		BatchedVertices.push_back(Vertex);
	}

	/** 라인 스트립 인덱스 생성 */
	GenerateLineStripIndices(CurrentVertexCount, VertexCount, bClosed);
	CurrentVertexCount += VertexCount;
}


void ULineBatchRenderer::UpdateVertexBuffer()
{
	if (!DynamicVertexBuffer || BatchedVertices.empty())
		return;

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT hr = DeviceContext->Map(DynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	if (SUCCEEDED(hr))
	{
		memcpy(MappedResource.pData, BatchedVertices.data(), CurrentVertexCount * sizeof(FVertex));
		DeviceContext->Unmap(DynamicVertexBuffer, 0);
	}
}

void ULineBatchRenderer::UpdateIndexBuffer()
{
	if (!DynamicIndexBuffer || BatchedIndices.empty())
		return;

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT hr = DeviceContext->Map(DynamicIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	if (SUCCEEDED(hr))
	{
		memcpy(MappedResource.pData, BatchedIndices.data(), CurrentIndexCount * sizeof(uint32));
		DeviceContext->Unmap(DynamicIndexBuffer, 0);
	}
}

void ULineBatchRenderer::RenderBatch()
{
	URenderer& Renderer = URenderer::GetInstance();

    // EditorPrimitive 생성하여 기존 렌더링 파이프라인 재사용 (인덱스 지원)
    FEditorPrimitive LinePrimitive;
    LinePrimitive.Vertexbuffer = DynamicVertexBuffer;
    LinePrimitive.NumVertices = CurrentVertexCount;
    LinePrimitive.IndexBuffer = DynamicIndexBuffer;
    LinePrimitive.NumIndices = CurrentIndexCount;
    LinePrimitive.bUseIndex = (CurrentIndexCount > 0);
    LinePrimitive.Color = FVector4(1.0f, 1.0f, 1.0f, 0.0f);
    LinePrimitive.Location = FVector::ZeroVector;
    LinePrimitive.Rotation = FVector::ZeroVector;
    LinePrimitive.Scale = FVector::OneVector;

    Renderer.RenderEditorPrimitive(LinePrimitive, PipelineDescKeyLine);
}

void ULineBatchRenderer::GenerateLineIndices(uint32 StartVertex, uint32 VertexCount)
{
	// 정점 2개씩 페어로 라인 생성
	for (uint32 i = 0; i < VertexCount; i += 2)
	{
		if (i + 1 < VertexCount)
		{
			BatchedIndices.push_back(StartVertex + i);
			BatchedIndices.push_back(StartVertex + i + 1);
			CurrentIndexCount += 2;
		}
	}
}

void ULineBatchRenderer::GenerateLineStripIndices(uint32 StartVertex, uint32 VertexCount, bool bClosed)
{
	// 연속된 정점들을 라인으로 연결
	for (uint32 i = 0; i < VertexCount - 1; ++i)
	{
		BatchedIndices.push_back(StartVertex + i);
		BatchedIndices.push_back(StartVertex + i + 1);
		CurrentIndexCount += 2;
	}

	// 닫힌 형태라면 마지막 정점을 첫 정점과 연결
	if (bClosed && VertexCount > 2)
	{
		BatchedIndices.push_back(StartVertex + VertexCount - 1);
		BatchedIndices.push_back(StartVertex);
		CurrentIndexCount += 2;
	}
}

