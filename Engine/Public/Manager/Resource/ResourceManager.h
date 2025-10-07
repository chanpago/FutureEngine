#pragma once
#include "Core/Object.h"
class UStaticMesh;

class UResourceManager : public UObject
{
	DECLARE_CLASS(UResourceManager, UObject)
	DECLARE_SINGLETON(UResourceManager)

public:
	void Initialize();

	void Release();

	UStaticMesh* GetStaticMesh(const FString& Path);

	TArray<FVertex>* GetVertexData(EPrimitiveType Type);
	ID3D11Buffer* GetVertexBuffer(EPrimitiveType Type);
	uint32 GetVertexNum(EPrimitiveType Type);


	void CreateTextSampler();
	ID3D11ShaderResourceView* LoadTexture(const FString& Path);
	ID3D11ShaderResourceView* GetTexture(const FString& Path);

	ID3D11SamplerState* GetSamplerState(ESamplerType Type);
	const FShader& GetShader(EShaderType Type) { return Shaders[Type]; }

	int32 GetCharInfoIdx(WCHAR Char);
	const TArray<FCharacterInfo>& GetCharInfos();

private:
	void LoadCharInfoMap();
	void CreateStaticMeshShader();
	void CreateDefaultShader();
	void CreateTextShader();

	void ReleaseShaders();

	TArray<FString> DefaultAssetPaths = { "Data/cube-tex.obj", "Data/triangle.obj", "Data/square.obj", "Data/sphere.obj", "Data/minion.obj", "Data/trees9.obj"};
	TMap<FString, UStaticMesh*> StaticMeshes;

	TMap<FString, ID3D11ShaderResourceView*> ShaderResourceViews;
	TMap<ESamplerType, ID3D11SamplerState*> SamplerStates;
	
	TMap<EShaderType, FShader> Shaders;





	////////////////////////////////////////////For Gizmo////////////////////////////////////
	TMap<EPrimitiveType, TArray<FVertex>*> VertexData;
	TMap<EPrimitiveType, ID3D11Buffer*> VertexBuffers;
	TMap<EPrimitiveType, uint32> VertexNum;
	////////////////////////////////////////////For Gizmo////////////////////////////////////


	TArray<FCharacterInfo> CharInfos;
	TMap<WCHAR, int32> CharInfoIdxMap;

	
};
