#pragma once
#include <d3d11.h>
#include "Global/Vector.h"

struct FEditorPrimitive
{
    ID3D11Buffer* Vertexbuffer;
    uint32 NumVertices;
    // Optional indexed draw support
    ID3D11Buffer* IndexBuffer = nullptr;
    uint32 NumIndices = 0;
    bool bUseIndex = false;
    D3D11_PRIMITIVE_TOPOLOGY Topology;
    FVector4 Color;
    FVector Location;
    FVector Rotation;
    FVector Scale;
	FRenderState RenderState;
	bool bShouldAlwaysVisible = false;
};
