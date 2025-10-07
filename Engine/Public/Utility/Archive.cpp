#include "pch.h"
#include "Archive.h"

FArchive& operator<<(FArchive& Ar, int8& Value)
{
	Ar.Serialize(&Value, sizeof(int8));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, int16& Value)
{
	Ar.Serialize(&Value, sizeof(int16));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, int32& Value)
{
	Ar.Serialize(&Value, sizeof(int32));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, int64& Value)
{
	Ar.Serialize(&Value, sizeof(int64));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, uint8& Value)
{
	Ar.Serialize(&Value, sizeof(uint8));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, uint16& Value)
{
	Ar.Serialize(&Value, sizeof(uint16));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, uint32& Value)
{
	Ar.Serialize(&Value, sizeof(uint32));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, uint64& Value)
{
	Ar.Serialize(&Value, sizeof(uint64));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, float& Value)
{
	Ar.Serialize(&Value, sizeof(float));
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FString& Value)
{
	if (Ar.IsSaving())
	{
		int32 Length = Value.length();
		Ar << Length;
		if (Length > 0)
		{
			Ar.Serialize((void*)Value.data(), Length * sizeof(char));
		}
	}
	else if (Ar.IsLoading())
	{
		int32 Length = 0;
		Ar << Length;
		if (Length > 0)
		{
			Value.resize(Length);
			Ar.Serialize((void*)Value.data(), Length * sizeof(char));
		}
		else
		{
			Value.clear();
		}
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FVector2& Value)
{
	Ar << Value.X;
	Ar << Value.Y;	
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FVector& Value)
{
	Ar << Value.X;
	Ar << Value.Y;
	Ar << Value.Z;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FVector4& Value)
{
	Ar << Value.X;
	Ar << Value.Y;
	Ar << Value.Z;
	Ar << Value.W;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FNormalVertex& Value)
{
	Ar << Value.Position;
	Ar << Value.Normal;
	Ar << Value.Color;
	Ar << Value.Tex;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FMeshSection& Value)
{
	Ar << Value.Name;
	Ar << Value.MaterialName;
	Ar << Value.IndexStart;
	Ar << Value.IndexCount;
	return Ar;
}

template<typename T>
FArchive& operator<<(FArchive& Ar, TArray<T>& Value)
{
	if (Ar.IsSaving())
	{
		int32 Size = Value.Num();
		Ar << Size;

		for (int32 i = 0; i < Size; i++)
		{
			Ar << Value[i];
		}
	}
	else if (Ar.IsLoading())
	{
		int32 Size = 0;
		Ar << Size;
		Value.resize(Size);
		for (int32 i = 0; i < Size; i++)
		{
			Ar << Value[i];
		}
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FStaticMesh& Value)
{
	Ar << Value.PathFileName;
	Ar << Value.Mtllib;

	// TArray 저장은 배열 크기 + 배열 요소
	Ar << Value.Vertices;

	// Read 시 IndexNum과 Indices의 개수를 비교해서 오류 검출
	Ar << Value.IndexNum;
	Ar << Value.Indices;

	if (Ar.IsLoading() && Value.IndexNum != Value.Indices.Num())
	{
		assert("StaticMesh .bin Indices size mismatch ");
	}

	Ar << Value.Sections;


	return Ar;
}

FArchive& operator<<(FArchive& Ar, FObjMaterialInfo& Value)
{
	Ar << Value.MaterialName;
	Ar << Value.Ka;
	Ar << Value.Kd;
	Ar << Value.Ks;
	Ar << Value.Ke;
	Ar << Value.Ns;
	Ar << Value.Ni;
	Ar << Value.d;
	Ar << Value.illum;
	Ar << Value.Map_Kd;
	Ar << Value.Map_Ks;
	Ar << Value.Map_bump;

	return Ar;
}
