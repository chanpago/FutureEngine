#include "pch.h"
#include "Editor/Axis.h"
#include "Render/Renderer/Renderer.h"
#include "Render/Renderer/LineBatchRenderer.h"

IMPLEMENT_CLASS(UAxis, UObject)

UAxis::UAxis()
{
	URenderer& Renderer = URenderer::GetInstance();
	AxisVertices.push_back({{ 50000.0f,0.0f,0.0f }, { 1,0,0,1 }});
	AxisVertices.push_back({ { 0.0f,0.0f,0.0f }, { 1,0,0,1 } });

	AxisVertices.push_back({ { 0.0f,50000.0f,0.0f }, { 0,1,0,1 } });
	AxisVertices.push_back({ { 0.0f,0.0f,0.0f }, { 0,1,0,1 } });

	AxisVertices.push_back({ { 0.0f,0.0f,50000.0f }, { 0,0,1,1 } });
	AxisVertices.push_back({ { 0.0f,0.0f,0.0f }, { 0,0,1,1 } });

	Primitive.NumVertices = static_cast<int>(AxisVertices.size());
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(AxisVertices);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Color = FVector4(1, 1, 1, 0);
	Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);
}

UAxis::~UAxis()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
}

// void UAxis::Render()
// {
// 	URenderer& Renderer = URenderer::GetInstance();
// 	Renderer.RenderEditorPrimitive(Primitive, Primitive.RenderState);
// }

void UAxis::AddToLineBatch(ULineBatchRenderer& LineBatch)
{
	/** Axis 라인들을 배칭에 추가 */
	LineBatch.AddLines(AxisVertices);
}
