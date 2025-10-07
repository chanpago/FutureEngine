#include "pch.h"
#include "Math/Bvh.h"
#include <algorithm>

FBvh::FBvh(const TArray<FVector>& InPositionList, const TArray<uint32>& InIndexList)
{
	TArray<FTriangleInfo> TriangleInfoList;
	TriangleInfoList.reserve(InIndexList.Num() / 3);
	NodeList.reserve(InIndexList.Num()*2);
	//모든 삼각형에 대해 중점과 AABB박스 계산해서 리스트에 저장.
	//TriangleInfoList[n]은 인덱스리스트에서 n번째 삼각형 정보 가리킴
	//InPositionList[InIndexList[n*3]] = n번째 삼각형 정점 중 하나
	for (int Index = 0; Index < InIndexList.Num(); Index+=3)
	{
		FVector Vertex0 = InPositionList[InIndexList[Index]];
		FVector Vertex1 = InPositionList[InIndexList[Index+1]];
		FVector Vertex2 = InPositionList[InIndexList[Index+2]];

		FAABB AABB;
		FVector Center = (Vertex0 + Vertex1 + Vertex2) * (1/3.0f);
		AABB.AddPoint(Vertex0);
		AABB.AddPoint(Vertex1);
		AABB.AddPoint(Vertex2);

		//
		TriangleInfoList.Add(FTriangleInfo{ AABB, Center, Index/3 });
	}

	DevideBvh(TriangleInfoList, 0, TriangleInfoList.Num());

	TriangleIndexList.reserve(TriangleInfoList.Num());
	for (int Index = 0; Index < TriangleInfoList.Num(); Index++)
	{
		TriangleIndexList.Add(TriangleInfoList[Index].OriginalTriangleIndex);
	}
}

int32 FBvh::DevideBvh(TArray<FTriangleInfo>& TriangleList, int32 LeftIndex, int32 RightIndex)
{
	int32 CurrentNodeIndex = NodeList.Emplace();

	int32 MidIndex;	//분할의 기준이 된

	//현재 노드 AABB결정. AABB말고 다른 정보는 알고리즘 적용하면서 알게됨

	for (int32 Index = LeftIndex; Index < RightIndex; Index++)
	{
		NodeList[CurrentNodeIndex].AABB.AddAABB(TriangleList[Index].AABB);
	}

	int32 TriangleCount = RightIndex - LeftIndex;
	//노드가 가진 Triangle개수가 Max이하면 더 이상 분할하지 않고 리턴
	if (TriangleCount <= TriangleInNodeMax)
	{
		NodeList[CurrentNodeIndex].bIsLeaf = true;
		NodeList[CurrentNodeIndex].Leaf.TriangleCount = TriangleCount;
		NodeList[CurrentNodeIndex].Leaf.TriangleStartIndex = LeftIndex;

		return CurrentNodeIndex;
	}
	//분할 시작
	else
	{
		NodeList[CurrentNodeIndex].bIsLeaf = false;
		MidIndex = CalculateMidIndex(CurrentNodeIndex, TriangleList, LeftIndex, RightIndex);
		//둘중 하나라도 같아지면 똑같은 인자로 계속 호출해서 스택 오버플로우
		if (MidIndex == LeftIndex || MidIndex == RightIndex)
		{
			NodeList[CurrentNodeIndex].bIsLeaf = true;
			NodeList[CurrentNodeIndex].Leaf.TriangleCount = TriangleCount;
			NodeList[CurrentNodeIndex].Leaf.TriangleStartIndex = LeftIndex;

			return CurrentNodeIndex;
		}
		NodeList[CurrentNodeIndex].Internal.LeftChild = DevideBvh(TriangleList, LeftIndex, MidIndex);
		NodeList[CurrentNodeIndex].Internal.RightChild = DevideBvh(TriangleList, MidIndex, RightIndex);
	}
	return CurrentNodeIndex;
}

int32 FBvh::CalculateMidIndex(const int32 CurrentNodeIndex, TArray<FTriangleInfo>& TriangleList, int32 LeftIndex, int32 RightIndex)
{
	//어느 축을 기준으로 어느 위치에서 자를지 결정, 그 이후 그 기준으로 파티션 나눔
	FAABB CurrentNodeAABB = NodeList[CurrentNodeIndex].AABB;
	FVector Size = CurrentNodeAABB.GetSize();
	int SplitAxis = (Size.X > Size.Y) ? (Size.Z > Size.X ? 2 : 0) : (Size.Y > Size.Z ? 1 : 2);

	//지금 AABB는 모두 모델 좌표계 원점 0,0 기준임. 이제 bin을 나눠서 최적의 분할 지점을 찾을 것임.
	float LowestCost = D3D11_FLOAT32_MAX;
	//축 위의 분할 위치. 이 위치로 분할해서 List 정렬하고 MidIndex를 구하는 것
	float SplitPosition = 0;	
	for (int Index = 1; Index <= BinNum; Index++)
	{
		//사이즈를 BinNum등분했을때 어느 지점으로 잘라야 가장 효율적인지 테스트
		//BinNum이 16일때 LeftBound의 첫 번째 대상은 MinX ~ MinX + Size.X/16까지.
		//RightBound는 LeftBound.X ~ MaxX
		float Position = CurrentNodeAABB.Min[SplitAxis] + Index /(float) BinNum * (Size[SplitAxis]);

		//Cost = (왼쪽 충돌 확률 * 왼쪽 삼각형 수 + 오른쪽 충돌 확률 * 오른쪽 삼각형 수)*삼각형 레이 충돌 계산 비용
		//삼각형 레이 충돌 계산 비용은 상수이고 전체 표면적도 상수이므로 제거하고 계산하면
		//Cost = (왼쪽 표면적 * 왼쪽 삼각형 수 + 오른쪽 표면적 * 오른쪽 삼각형 수)
		FAABB LeftAABB;
		FAABB RightAABB;
		int32 LeftCount = 0;
		int32 RightCount = 0;

		for (int TriIndex = LeftIndex; TriIndex < RightIndex; TriIndex++)
		{
			//이미 삼각형들이 현재 노드의 AABB박스 안에는 들어있다는 가정이므로 중심의 축 좌표만 비교하면 됨
			if (TriangleList[TriIndex].Center[SplitAxis] < Position)
			{
				LeftAABB.AddAABB(TriangleList[TriIndex].AABB);
				LeftCount++;
			}
			else
			{
				RightAABB.AddAABB(TriangleList[TriIndex].AABB);
				RightCount++;
			}
		}
		float Cost = D3D11_FLOAT32_MAX;
		if (LeftCount > 0 && RightCount > 0)
		{
			float Cost = LeftAABB.GetSurfaceArea() * LeftCount + RightAABB.GetSurfaceArea() * RightCount;
		}
		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			SplitPosition = Position;
		}
	}

	auto MidIndex = std::partition(TriangleList.begin() + LeftIndex, TriangleList.begin() + RightIndex,
		[&](const FTriangleInfo& Info)
		{
			return Info.Center[SplitAxis] < SplitPosition;
		});

	return std::distance(TriangleList.begin(), MidIndex);

}


bool FBvh::IsRayCollided(const FRay& ModelRay, const TArray<FVector>& Vertices, const TArray<uint32>& Indices)
{
	TArray<int32> NextNode;

	int32 CurrentNode;
	float ShortestDistance = D3D11_FLOAT32_MAX;
	float NewDistance;
	bool bIsHit = false;
	if (!FMath::IsRayCollidWithAABB(ModelRay, NodeList[0].AABB, NewDistance))
	{
		return false;
	}
	NextNode.Add(0);
	while (!NextNode.IsEmpty())
	{
		CurrentNode = NextNode.Pop();

		
		//리프노드일 경우 인덱스 리스트 추가하고 다른 노드 더 확인(Bvh는 노드끼리 겹칠 수 있음, 삼각형의 AABB를 합쳐서 계산하므로)
		if (NodeList[CurrentNode].bIsLeaf)
		{
			for (int Index = 0; Index < NodeList[CurrentNode].Leaf.TriangleCount; Index++)
			{
				int TriangleIndex = TriangleIndexList[NodeList[CurrentNode].Leaf.TriangleStartIndex + Index];
				const FVector& Vertex1 = Vertices[Indices[TriangleIndex* 3]];
				const FVector& Vertex2 = Vertices[Indices[TriangleIndex* 3 + 1]];
				const FVector& Vertex3 = Vertices[Indices[TriangleIndex* 3 + 2]];

				if (FMath::IsRayTriangleCollided(ModelRay, Vertex1, Vertex2, Vertex3, &NewDistance))
				{
					bIsHit = true;
					if (NewDistance < ShortestDistance)
					{
						ShortestDistance = NewDistance;
					}
				}
			}

			
		}
		else
		{
			bool bLeftHit = false;
			bool bRightHit = false;
			float LeftDistance;
			float RightDistance;
			if (FMath::IsRayCollidWithAABB(ModelRay, NodeList[NodeList[CurrentNode].Internal.LeftChild].AABB, LeftDistance))
			{
				if (LeftDistance < ShortestDistance)
				{
					bLeftHit = true;
				}
			}
			if (FMath::IsRayCollidWithAABB(ModelRay, NodeList[NodeList[CurrentNode].Internal.RightChild].AABB, RightDistance))
			{
				if (RightDistance < ShortestDistance)
				{
					bRightHit = true;
				}
			}
			if (bLeftHit && bRightHit)
			{
				if (LeftDistance < RightDistance)
				{
					NextNode.Add(NodeList[CurrentNode].Internal.RightChild);
					NextNode.Add(NodeList[CurrentNode].Internal.LeftChild);
				}
				else
				{
					NextNode.Add(NodeList[CurrentNode].Internal.LeftChild);
					NextNode.Add(NodeList[CurrentNode].Internal.RightChild);
				}
			}
			else if (bLeftHit)
			{
				NextNode.Add(NodeList[CurrentNode].Internal.LeftChild);
			}
			else if(bRightHit)
			{
				NextNode.Add(NodeList[CurrentNode].Internal.RightChild);
			}
			
		}
	}
	return bIsHit;
}
