#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include <cstdint>

struct FViewProjConstants
{
	FViewProjConstants()
	{
		View = FMatrix::Identity;
		Projection = FMatrix::Identity;
		ViewModeIndex = 0;
	}

	FMatrix View;
	FMatrix Projection;
	uint32 ViewModeIndex;
};


struct FVertex
{
    FVector Position;
    FVector4 Color;

	FVertex() : Position{}, Color{} {}

	FVertex(const FVector& pos, const FVector4& col) : Position(pos), Color(col) {}

    // 동등 비교 연산자
    bool operator==(const FVertex& Other) const
    {
        return Position == Other.Position && Color == Other.Color;
    }
};

namespace std
{
	template<>
	struct hash<FVertex>
	{
		size_t operator()(const FVertex& V) const noexcept
		{
			auto HashCombine = [](size_t Seed, size_t Value) -> size_t
			{
				Seed ^= Value + 0x9e3779b97f4a7c15ULL + (Seed << 6) + (Seed >> 2);
				return Seed;
			};

			size_t H = 0;
			H = HashCombine(H, std::hash<float>{}(V.Position.X));
			H = HashCombine(H, std::hash<float>{}(V.Position.Y));
			H = HashCombine(H, std::hash<float>{}(V.Position.Z));
			H = HashCombine(H, std::hash<float>{}(V.Color.X));
			H = HashCombine(H, std::hash<float>{}(V.Color.Y));
			H = HashCombine(H, std::hash<float>{}(V.Color.Z));
			H = HashCombine(H, std::hash<float>{}(V.Color.W));
			return H;
		}
	};
}

struct FTextVertex
{
	FVector Position;
	FVector2 BaseUv;
	uint32 CharId;
};

struct FCharacterInfo
{
	FVector2 UvOffset;
	FVector2 UvSize;
};

#pragma region objstruct
struct FVertexKey
{
	int32 v, vt, vn;
	bool operator==(const FVertexKey& k) const
	{
		return v == k.v && vt == k.vt && vn == k.vn;
	}
};

struct FVertexKeyHash
{
	size_t operator() (const FVertexKey& k) const noexcept
	{
		size_t h = 1469598103934665603ull;
		auto mix = [&](int32 x)
			{
				h ^= size_t(x + 1);
				h *= 1099511628211ull;
			};
		mix(k.v);
		mix(k.vt);
		mix(k.vn);

		return h;
	}
};

struct FObjImportOption
{
	bool bIsFlipWinding = false;
	bool bIsInvertTexV = false;
	// vn이 없는 경우
	bool bIsRecalculateNormals = true;
};

struct FObjVertexRef
{
	int32 V = -1;
	int32 VT = -1;
	int32 VN = -1;
};

struct FObjFace
{
	TArray<FObjVertexRef> Conners;
};

struct FObjSection
{
	FString Name;
	FString MaterialName;
	TArray<FObjFace> Faces;
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 Tex;

	FNormalVertex()
		: Position(FVector(-1.0f,-1.0f,-1.0f)), Normal(FVector(-1.0f, -1.0f, -1.0f)),
		Color(FVector4(-1.0f, -1.0f, -1.0f, 1.0f)), Tex(FVector2(-1.0f, -1.0f)) { }
	FNormalVertex(const FVector& v, const FVector& norm, const FVector4& c, const FVector2& uv)
		: Position(v), Normal(norm), Color(c), Tex(uv) {}
};

struct FObjInfo
{
	// Vertex
	TArray<FVector> Position;
	TArray<FVector> Normal;
	TArray<FVector4> Color; // Material 사용 시 필요할듯
	TArray<FVector2> Tex;
	FString Mtllib;
	TArray<FObjSection> Sections;

	FObjInfo()
		: Position{}, Normal{}, Color{}, Tex{} {}		
};

struct FMeshSection
{
	FString Name;
	FString MaterialName;
	uint32 IndexStart = 0;
	uint32 IndexCount = 0;
};

// Cooked Data
struct FStaticMesh
{
	FString PathFileName;
	FString Mtllib;

	TArray<FNormalVertex> Vertices;
	TArray<uint32> Indices;
	TArray<FMeshSection> Sections;
	uint32 IndexNum;	

	FStaticMesh() : PathFileName{}, Vertices{}, Indices{}, Sections{}, IndexNum(0) {}

	FStaticMesh(const FString& name, const TArray<FNormalVertex>& vertices, const TArray<uint32>& indices,
		const TArray<FMeshSection>& sections, uint32 indexNum)
		: PathFileName(name), Vertices(vertices), Indices(indices), Sections{}, IndexNum(indexNum) {}
};

struct FObjMaterialInfo
{
	FString MaterialName;
	FVector Ka, Kd, Ks, Ke;
	float Ns, Ni, d;
	uint32 illum;
	FString Map_Kd;
	FString Map_Ks;
	FString Map_bump;

	FObjMaterialInfo() : Ka{}, Kd{}, Ks{}, Ke{}, Ns(0), Ni(0), d(0), illum(-1) {}
	FObjMaterialInfo(const FVector& _Ka, const FVector& _Kd, const FVector& _Ks, const FVector& _Ke,
		float _Ns, float _Ni, float _d, uint32 _illum)
		:Ka(_Ka), Kd(_Kd), Ks(_Ks), Ke(_Ke), Ns(_Ns), Ni(_Ni), d(_d), illum(_illum) {}
};

enum class EFileFormat : uint8
{
	EFF_Obj,
	EFF_Mtl
};
#pragma endregion

//TMap<char, FCharacterInfo> CharInfoMap;

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Component Type Enum
 */
enum class EComponentType : uint8_t
{
	None = 0,

	Actor,
		//ActorComponent Dervied Type

	Scene,
		//SceneComponent Dervied Type

	Primitive,

	StaticMesh,

	Text,

	Billboard,
		//PrimitiveComponent Derived Type

	End = 0xFF
};

/**
 * @brief UObject Primitive Type Enum
 */
enum class EPrimitiveType : uint8_t
{
	None = 0,
	Sphere,
	Cube,
	Triangle,
	Square,
	Arrow,
	CubeArrow,
	Ring,
	StaticMeshComp,


	End = 0xFF
};

/**
 * @brief View Mode State
 */
enum class EViewModeIndex : uint32
{
	Lit,
	Unlit,
	Wireframe,

	End
};

enum class ESamplerType
{
	Text,
};

/**
 * @brief Engine Show Flags for controlling visibility of various elements
 */
enum class EEngineShowFlags : uint64
{
	SF_None = 0,
	SF_Primitives = 1 << 0,      // Show primitive objects
	SF_BillboardText = 1 << 1,   // Show billboard text
	SF_Grid = 1 << 2,            // Show grid
	SF_Bounds = 1 << 3,          // Show bounding boxes

	// Default flags (everything visible)
	SF_Default = SF_Primitives | SF_BillboardText | SF_Grid
};

// Bitwise operators for EEngineShowFlags
inline EEngineShowFlags operator|(EEngineShowFlags a, EEngineShowFlags b)
{
	return static_cast<EEngineShowFlags>(static_cast<uint64>(a) | static_cast<uint64>(b));
}

inline EEngineShowFlags operator&(EEngineShowFlags a, EEngineShowFlags b)
{
	return static_cast<EEngineShowFlags>(static_cast<uint64>(a) & static_cast<uint64>(b));
}

inline EEngineShowFlags operator^(EEngineShowFlags a, EEngineShowFlags b)
{
	return static_cast<EEngineShowFlags>(static_cast<uint64>(a) ^ static_cast<uint64>(b));
}

inline EEngineShowFlags operator~(EEngineShowFlags a)
{
	return static_cast<EEngineShowFlags>(~static_cast<uint64>(a));
}

inline EEngineShowFlags& operator|=(EEngineShowFlags& a, EEngineShowFlags b)
{
	return a = a | b;
}

inline EEngineShowFlags& operator&=(EEngineShowFlags& a, EEngineShowFlags b)
{
	return a = a & b;
}

inline EEngineShowFlags& operator^=(EEngineShowFlags& a, EEngineShowFlags b)
{
	return a = a ^ b;
}

inline bool HasFlag(EEngineShowFlags flags, EEngineShowFlags flag)
{
	return (flags & flag) == flag;
}

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	D3D11_CULL_MODE CullMode = D3D11_CULL_NONE;
	D3D11_FILL_MODE FillMode = D3D11_FILL_SOLID;
};

enum EWorldType
{
	Editor,
	EditorPreview,
	PIE,
	Game,
};

class UWorld;
struct FWorldContext
{ 
	// Type EnumType
	EWorldType WorldType;

	/** The Prefix in front of PIE level names, empty is default*/
	FString PIEPrefix;

	/** The PIE instance of the world, -1 is default*/
	int32 PIEInstance;

	FORCEINLINE UWorld* World() const
	{
		return ThisCurrentWorld;
	}

	bool operator==(const FWorldContext& rhs) const { 
		return WorldType == rhs.WorldType;
	}

	UWorld* World() { return ThisCurrentWorld; };
	void SetWorld(UWorld* World) { ThisCurrentWorld = World; }
private:
	UWorld* ThisCurrentWorld;

};


/**
 * @brief 2D 정수 좌표
 */
struct FPoint
{
	INT X, Y;

	FPoint() = default;

	constexpr FPoint(LONG InX, LONG InY) : X(InX), Y(InY)
	{
	}
};

/**
 * @brief 2D 정수 사각형
 */
struct FRect
{
	LONG X, Y, W, H;

	FRect() = default;

	constexpr FRect(LONG InX, LONG InY, LONG InW, LONG InH) : X(InX), Y(InY), W(InW), H(InH)
	{
	}
};
