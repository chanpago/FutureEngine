#pragma once

struct FAABB;

struct FBvh
{
public:
	struct FNode
	{
		FAABB AABB;
		bool bIsLeaf;
		union
		{
			struct
			{
				int32 LeftChild;
				int32 RightChild;
			}Internal;
			struct
			{
				int32 TriangleStartIndex;
				int32 TriangleCount;
			}Leaf;
		};
	};

	FBvh(const TArray<FVector>& InPositionList, const TArray<uint32>& InIndexList);
	bool IsRayCollided(const FRay& ModelRay, const TArray<FVector>& Vertices, const TArray<uint32>& Indices);

private:
	struct FTriangleInfo
	{
		FAABB AABB;	//삼각형을 감싸는 AABB
		FVector Center;	//삼각형 중심
		int32 OriginalTriangleIndex;	//정렬 후에 이걸로 리스트를 만들어야함
	};

	//TriangleList에서 StartIndex와 EndIndex 가지고 분할. 리턴값은 노드 인덱스
	int32 DevideBvh( TArray<FTriangleInfo>& TriangleList, int32 LeftIndex, int32 RightIndex);
	int32 CalculateMidIndex(const int32 CurrentNodeIndex, TArray<FTriangleInfo>& TriangleList, int32 LeftIndex, int32 RightIndex);

	




	TArray<FNode> NodeList;
	//InPositionList[IndexList[TriangleIndexList[0]*3]], InPositionList[IndexList[TriangleindexList[0]*3+1]]...+2]] = 삼각형 하나
	TArray<uint32> TriangleIndexList;

	const int32 TriangleInNodeMax = 4;
	const int32 BinNum = 16;
};
