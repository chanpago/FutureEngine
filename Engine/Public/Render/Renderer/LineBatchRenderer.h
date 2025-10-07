#pragma once
#include "Core/Object.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"

/**
 * @brief Line Batch Renderer - 모든 라인 렌더링 요청을 배칭하여 단일 드로우콜로 처리
 *
 * D3D11_PRIMITIVE_TOPOLOGY_LINELIST 사용
 * 동적 Vertex Buffer와 Index Buffer를 통한 효율적인 배칭
 * Grid, Axis, Gizmo 등 모든 라인 렌더링을 통합 관리
 */
class ULineBatchRenderer : public UObject
{
    DECLARE_CLASS(ULineBatchRenderer, UObject)
    DECLARE_SINGLETON(ULineBatchRenderer)

public:
    void Init();
    void Release();

	/** 배칭 관리 */
	void BeginBatch();
	void FlushBatch();
	void ClearBatch();

	/** 라인 추가 인터페이스 */
    void AddLine(const FVector& Start, const FVector& End, const FVector4& Color);
    void AddLines(const TArray<FVertex>& Vertices);
    void AddLineStrip(const TArray<FVertex>& Vertices, bool bClosed = false);
    void AddIndexedLines(const TArray<FVertex>& Vertices, const TArray<uint32>& Indices);

    /** 유틸리티 */
    uint32 GetCurrentVertexCount() const { return CurrentVertexCount; }
    uint32 GetMaxVertices() const { return MaxVertices; }
    bool CanAddVertices(uint32 Count) const { return CurrentVertexCount + Count <= MaxVertices; }

private:
	/** DirectX 11 리소스 */
    ID3D11Buffer* DynamicVertexBuffer = nullptr;
    ID3D11Buffer* DynamicIndexBuffer = nullptr;

	FPipelineDescKey PipelineDescKeyLine;

    /** 배칭 데이터 */
    TArray<FVertex> BatchedVertices;
    TArray<uint32> BatchedIndices;


    /** 배치 설정 */
    static constexpr uint32 MaxVertices = 10000;
    static constexpr uint32 MaxIndices = MaxVertices * 2; // Line topology: 2 indices per line

    /** 현재 상태 */
    uint32 CurrentVertexCount = 0;
    uint32 CurrentIndexCount = 0;
    bool bBatchStarted = false;

	/** 내부 메서드 */
    void CreateBuffers();
    void ReleaseBuffers();
    void UpdateVertexBuffer();
    void UpdateIndexBuffer();
    void RenderBatch();

    /** 인덱스 생성 */
    void GenerateLineIndices(uint32 StartVertex, uint32 VertexCount);
    void GenerateLineStripIndices(uint32 StartVertex, uint32 VertexCount, bool bClosed);
};
