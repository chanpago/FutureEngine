#pragma once
//TTrait : T type에 어떤 멤버가 있을지 모름, 그걸 옥트리에서 가정하고 작성하면 나중에 수정할때 옥트리를 수정해야함
//그 가정을 옥트리가 아닌 TTrait에 작성해서 넘겨주면 나중에 T의 멤버 이름이 바뀌거나 옥트리를 사용하는 아예 다른 구조의 Type이 추가됐을때
//TTrait을 그에 맞게 새로 만들거나 한 줄만 수정하면 되서 옥트리 클래스는 옥트리에만 집중할 수 있음.
#include "Math/Math.h"

class UCamera;

template<typename T, typename TTrait>
class TOctree
{

public:
	struct FOctreeNode
	{
		//AABB말고 다른 모양도 노드에 들어올 수 있음. 일단은 AABB만 처리하도록 함.
		FAABB AABB;

		//ChildIndex가 -1이면 자식 존재 X, 리프노드
		int32 ChildStartIndex = -1;
		int32 ParentIndex = -1;
		

		//노드가 객체를 저장하고 있으면 객체끼리 메모리에 불연속적으로 위치하게 되므로 spatial locality가 낮아짐
		//객체들은 TOctree가 통째로 저장하고 node는 어디서부터 어디까지의 객체를 본인이 포함하고 있는지 알려주면 됨.
		uint32 Depth = 1;
		uint32 ElementStartIndex = 0;
		uint32 ElementCount = 0;

		//처음엔 index상관 없이 삽입하고 나중에 재정렬 할 것이라서 임시 공간이 필요함
		TArray<T*> TemporalElements;
	};


	TOctree()
	{
		FOctreeNode RootNode;
		RootNode.AABB = FAABB(FVector(-50, -50, -50), FVector(50, 50, 50));
		OctreeNodes.Add(RootNode);
	}

	//사이즈를 지정하고 싶을때는 무조건 NewOctree로 지정 후 사용
	void NewOctree(const FAABB& OctreeSize)
	{
		clear();
		FOctreeNode RootNode;
		RootNode.AABB = OctreeSize;
		OctreeNodes.Add(RootNode);

	}


	void clear()
	{
		OctreeNodes.clear();
		Elements.clear();
		ElementsOutsideOctree.clear();
		FOctreeNode RootNode;
		RootNode.AABB = FAABB(FVector(-50, -50, -50), FVector(50, 50, 50));
		OctreeNodes.Add(RootNode);
	}

	TArray<FOctreeNode>& GetOctreeNodes()
	{
		return OctreeNodes;
	}
	TArray<T*>& GetElementList() 
	{
		return Elements;
	}

	const TArray<T*>& GetElementsOutsideOctree() const
	{
		return ElementsOutsideOctree;
	}

	void RemoveElement(T* Element)
	{
		int32 CurrentNodeIndex = TTrait::GetOctreeIndex(Element);
		int32 OutsideIndex = -1;
		if (CurrentNodeIndex == -1)
		{
			if ((OutsideIndex = ElementsOutsideOctree.Find(Element)) != -1)
			{
				ElementsOutsideOctree.RemoveAt(OutsideIndex);
			}
			return;
		}
		int32 ElementIndex = GetElementIndexInNode(CurrentNodeIndex, Element);
		if (ElementIndex != -1)
		{
			//마지막 원소를 제거할 원소의 위치로 옮기고 카운트 다운함. 제거하면 나머지 트리가 참조를 못해서 일단 냅두고 나중에 재정렬함
			Elements[ElementIndex] = Elements[OctreeNodes[CurrentNodeIndex].ElementStartIndex + OctreeNodes[CurrentNodeIndex].ElementCount - 1];
			OctreeNodes[CurrentNodeIndex].ElementCount--;
		}
		ElementIndex = GetTemporalIndexInNode(CurrentNodeIndex, Element);

		if (ElementIndex != -1)
		{
			OctreeNodes[CurrentNodeIndex].TemporalElements.RemoveAt(ElementIndex);
		}
	}

	void UpdateElement(T* Element)
	{
		/*UE_LOG("InOctree : %d, OutsizeOctree : %d, TempNum : %d", Elements.Num(), ElementsOutsideOctree.Num(), TemporalElementNum);
		UE_LOG("Element is in %d", TTrait::GetOctreeIndex(Element));*/
		int32 CurrentNodeIndex = TTrait::GetOctreeIndex(Element);
		//ElementsOutsideOctree에 있거나 애초에 옥트리에 포함되면 안되는 element
		if (CurrentNodeIndex == -1)
		{
			int32 OutsideIndex;
			if ((OutsideIndex = ElementsOutsideOctree.Find(Element)) == -1)
			{
				return;
			}
			else
			{
				ElementsOutsideOctree.RemoveAt(OutsideIndex);
				AddElement(Element,0);
				return;
			}
		}

		FAABB ElementBound = TTrait::GetWorldAABB(Element);
		FAABB AABBExpanded = OctreeNodes[CurrentNodeIndex].AABB;
		AABBExpanded.ScaleBy(1.2f);
		//element의 AABB가 자기가 위치하던 노드를 벗어나지 않은 경우
		if (AABBExpanded.Contains(ElementBound) && OctreeNodes[CurrentNodeIndex].AABB.Contains(TTrait::GetPosition(Element)))
		{
			return;
		}
		else
		{
			RemoveElement(Element);

			/*if (!bIsRemoved)
				return;*/
			//현재 노드를 벗어났으므로 부모노드로 이동
			CurrentNodeIndex = OctreeNodes[CurrentNodeIndex].ParentIndex;	
			while (CurrentNodeIndex != -1)
			{
				int32 ChildIndex = GetChildIndexForAABB(ElementBound, Element, CurrentNodeIndex);
				//형제 노드들 중에서 본인을 포함하는 노드 찾음
				if (ChildIndex != -1)
				{
					OctreeNodes[ChildIndex].TemporalElements.Add(Element);
					TTrait::SetOctreeIndex(Element, ChildIndex);
					if (OctreeNodes[ChildIndex].ChildStartIndex == -1 &&
						(OctreeNodes[ChildIndex].ElementCount + OctreeNodes[ChildIndex].TemporalElements.Num()) > OctreeNodeMax &&
						OctreeNodes[ChildIndex].Depth < MaxDepth)
					{
						DevideOctreeNode(ChildIndex, ElementBound);
					}

					
					return;
				}
				//형제들도 본인을 포함 안함, 다시 부모노드로 이동
				else
				{
					CurrentNodeIndex = OctreeNodes[CurrentNodeIndex].ParentIndex;
				}
			}
			//루트노드마저도 포함 안함.
			ElementsOutsideOctree.Add(Element);
			TTrait::SetOctreeIndex(Element, -1);
		}
	}
	//어느 옥트리 노드에 들어갈지, 그 노드가 꽉 찼는지, 분할을 해야하는지 테스트 필요
	//Element : 집어넣을 element
	//CurrentNodeIndex : 어느 노드부터 시작할지 설정
	//Depth : 그 노드의 Depth, 최대 depth가 없으면 한 좌표에 물체가 임계점 이상 모여있으면 무한히 분할함.
	void AddElement(T* Element, uint32 CurrentNodeIndex)
	{
		bIsElementsDirty = true;
		int32 ChildIndex = -1;
		FAABB Bound = TTrait::GetWorldAABB(Element);

		if (CurrentNodeIndex == 0 && !OctreeNodes[CurrentNodeIndex].AABB.Contains(Bound))
		{
			ElementsOutsideOctree.Add(Element);
			return;
		}
		//옥트리의 노드를 순회, 리프노드면(childIndex가 -1) 그 노드에 들어가면 되고 아니면 딱 맞는 노드를 찾아줘야함
		//딱 맞는 노드가 없어도 -1을 반환, 리프노드일때와 아닐때를 따로 처리해줘야함
		while ((ChildIndex = GetChildIndexForAABB(Bound, Element, CurrentNodeIndex)) != -1)
		{
			CurrentNodeIndex = ChildIndex;
		}

		//리프노드든 아니든 일단 넣고 꽉차면 분할함
		OctreeNodes[CurrentNodeIndex].TemporalElements.push_back(Element);
		TTrait::SetOctreeIndex(Element, CurrentNodeIndex);

		//MaxDepth까지 내려오면 더 분할 안 함
		if (OctreeNodes[CurrentNodeIndex].Depth >= MaxDepth)
		{
			return;
		}
		//리프노드이고 임계점 도달 시 분할함
		if (OctreeNodes[CurrentNodeIndex].ChildStartIndex == -1 &&
			(OctreeNodes[CurrentNodeIndex].ElementCount + OctreeNodes[CurrentNodeIndex].TemporalElements.Num()) > OctreeNodeMax)
		{
			DevideOctreeNode(CurrentNodeIndex, Bound);
		}
	}

	//노드를 add할때 그냥 ElementIndex 신경 안 쓰고 삽입했기 때문에
	//노드의 element들이 연속된 메모리에 위치하도록 재정렬하는 함수
	void Rearrange()
	{
		if (bIsElementsDirty)
		{
			static float TimeAfterCal;
			TimeAfterCal += DT;
			if (TimeAfterCal >= 2)
			{
				TemporalElementNum = 0;
				for (int Index = 0; Index < OctreeNodes.Num(); Index++)
				{
					TemporalElementNum += OctreeNodes[Index].TemporalElements.Num();
				}
				TimeAfterCal = 0;
			}

			if (TemporalElementNum > MaxTemporalElementNum)
			{

				TArray<T*> NewElements;
				NewElements.reserve(Elements.Num() + TemporalElementNum);

				//NewElements List에 노드의 temporalElements와 기존의 element를 DFS로 트리를 순회하면서 배치함
				DfsOctree(0, NewElements);

				Elements = NewElements;
				bIsElementsDirty = false;
				TemporalElementNum = 0;
			}
		}
	}

	T* GetCollidedElement(const FRay& WorldRay, float& ShortestDistance)
	{
		//트리 순회하면서 충돌한 노드가 가진 객체 충돌테스트 후 거리 저장.
		//저장된 거리보다 거리가 긴 노드는 버림.
		T* PickedElement = nullptr;
		std::priority_queue<std::pair<float, uint32>, std::vector<std::pair<float, uint32>>, std::greater<std::pair<float, uint32>>> NextNode;
		float NewDistance;
		if (FMath::IsRayCollidWithAABB(WorldRay, OctreeNodes[0].AABB, NewDistance))
		{
			NextNode.push({ NewDistance, 0});
		}
		
		ShortestDistance = D3D11_FLOAT32_MAX;

		while (!NextNode.empty())
		{
			std::pair Pair = NextNode.top();
			NextNode.pop();
			
			//현재까지 충돌판정된 실제 객체까지 거리보다 더 긴 노드에 대해서는 굳이 테스트할 필요가 없음
			if (Pair.first > ShortestDistance)
			{
				break;
			}
			for (int Index = 0; Index < OctreeNodes[Pair.second].ElementCount; Index++)
			{
				//실제 노드 안의 element들과 충돌 테스트.
				if (TTrait::IsRayCollided(Elements[OctreeNodes[Pair.second].ElementStartIndex + Index],WorldRay, NewDistance) && (NewDistance < ShortestDistance))
				{
					ShortestDistance = NewDistance;
					PickedElement = Elements[OctreeNodes[Pair.second].ElementStartIndex + Index];
				}
			}
			for (int Index = 0; Index < OctreeNodes[Pair.second].TemporalElements.Num(); Index++)
			{
				if (TTrait::IsRayCollided(OctreeNodes[Pair.second].TemporalElements[Index], WorldRay, NewDistance) && (NewDistance < ShortestDistance))
				{
					ShortestDistance = NewDistance;
					PickedElement = OctreeNodes[Pair.second].TemporalElements[Index];
				}
			}

			if (OctreeNodes[Pair.second].ChildStartIndex != -1)
			{
				for (int Index = 0; Index < 8; Index++)
				{
					//부모노드 테스트 끝났고 이제 자식노드 순회할 거임
					if (FMath::IsRayCollidWithAABB(WorldRay, OctreeNodes[OctreeNodes[Pair.second].ChildStartIndex + Index].AABB, NewDistance) && (NewDistance < ShortestDistance))
					{
						NextNode.push({NewDistance,OctreeNodes[Pair.second].ChildStartIndex + Index });
					}
				}
			}
		}
		for (int Index = 0; Index < ElementsOutsideOctree.Num(); Index++)
		{
			float Distance;
			if (ElementsOutsideOctree[Index]->IsRayCollided(WorldRay, Distance) && (Distance < ShortestDistance))
			{
				ShortestDistance = Distance;
				PickedElement = ElementsOutsideOctree[Index];
			}
		}
		

		return PickedElement;

	}

	void OctreeFrustumCulling(UCamera* Camera, TArray<T*>& PrimitiveComponentsToRender)
	{
		std::priority_queue<
			std::pair<float, uint32>,
			std::vector<std::pair<float, uint32>>,
			std::greater<std::pair<float, uint32>>
		> NextNode;


		if (!Camera->IsOnFrustum(OctreeNodes[0].AABB))
		{
			return;
		}
		FVector CameraLocation = Camera->GetLocation();
		float Distance = FMath::Dist2(CameraLocation, OctreeNodes[0].AABB.GetCenter());
		NextNode.push({ Distance, 0 });

		while (!NextNode.empty())
		{
			std::pair CurrentNodePair = NextNode.top();
			NextNode.pop();

			int ElementStartIndex = OctreeNodes[CurrentNodePair.second].ElementStartIndex;

			for (int Index = 0; Index < OctreeNodes[CurrentNodePair.second].ElementCount; Index++)
			{
				if (Camera->IsOnFrustum(Elements[ElementStartIndex + Index]))
				{
					PrimitiveComponentsToRender.Add(Elements[ElementStartIndex + Index]);
				}
			}

			for (int Index = 0; Index < OctreeNodes[CurrentNodePair.second].TemporalElements.Num(); Index++)
			{
				if (Camera->IsOnFrustum(OctreeNodes[CurrentNodePair.second].TemporalElements[Index]))
				{
					PrimitiveComponentsToRender.Add(OctreeNodes[CurrentNodePair.second].TemporalElements[Index]);
				}
			}

			int ChildStartIndex = -1;
			if ((ChildStartIndex = OctreeNodes[CurrentNodePair.second].ChildStartIndex) != -1)
			{
				for (int Index = 0; Index < 8; Index++)
				{
					if (Camera->IsOnFrustum(OctreeNodes[ChildStartIndex + Index].AABB))
					{
						Distance = FMath::Dist2(CameraLocation, OctreeNodes[ChildStartIndex + Index].AABB.GetCenter());
						NextNode.push({ Distance,ChildStartIndex + Index });
					}
				}
			}
		}
	}

private:
	void DfsOctree(uint32 CurrentNodeIndex, TArray<T*>& NewElements)
	{
		FOctreeNode& CurrentNode = OctreeNodes[CurrentNodeIndex];

		TArray<T*> NodeElements;
		CollectElementsFrom(CurrentNode, NodeElements);

		CurrentNode.ElementStartIndex = NewElements.Num();
		CurrentNode.ElementCount = NodeElements.Num();
		if (NodeElements.Num() > 0)
		{
			NewElements.Append(NodeElements);
		}
		CurrentNode.TemporalElements.clear();


		if (CurrentNode.ChildStartIndex != -1)
		{
			for (int Index = 0; Index < 8; Index++)
			{
				DfsOctree(CurrentNode.ChildStartIndex + Index, NewElements);
			}
		}
	}
	int32 GetChildIndexForAABB(const FAABB& Bound, T* Element, uint32 CurrentNodeIndex)
	{
		int32 ChildStartIndex = OctreeNodes[CurrentNodeIndex].ChildStartIndex;
		if (ChildStartIndex == -1)
			return -1;
		//8개의 자식 노드 순회하면서 AABB박스 테스트.
		for (int Index = 0; Index < 8; Index++)
		{
			FAABB ChildAABBExpanded = OctreeNodes[ChildStartIndex + Index].AABB;
			ChildAABBExpanded.ScaleBy(1.2f);
			//옥트리 노드가 Bound를 완전히 포함하는 경우 인덱스 바로 리턴
			if (ChildAABBExpanded.Contains(Bound) && OctreeNodes[ChildStartIndex + Index].AABB.Contains(TTrait::GetPosition(Element)))
			//if(OctreeNodes[CurrentNodeIndex].AABB.Contains(Bound))
			{
				return ChildStartIndex + Index;
			}
		}
		//자식들 중 어느 노드도 Bound를 포함하지 않으면 지금의 노드에 들어갈 것이므로 -1 리턴
		return -1;
	}

	int32 GetElementIndexInNode(uint32 CurrentNodeIndex, T* Element)
	{
		for (int Index = 0; Index < OctreeNodes[CurrentNodeIndex].ElementCount; Index++)
		{
			if (Elements[OctreeNodes[CurrentNodeIndex].ElementStartIndex + Index] == Element)
			{
				return OctreeNodes[CurrentNodeIndex].ElementStartIndex + Index;
			}
		}
		return -1;
	}

	int32 GetTemporalIndexInNode(uint32 CurrentNodeIndex, T* Element)
	{
		for (int Index = 0; Index < OctreeNodes[CurrentNodeIndex].TemporalElements.Num(); Index++)
		{
			if (OctreeNodes[CurrentNodeIndex].TemporalElements[Index] == Element)
			{
				return Index;
			}
		}
		return -1;
	}

	//분할 진행해서 element가 들어가야할 노드의 index반환(분할이 2회 이상 일어날 수도 있고 한번 일어난 다음 현재 node에 삽입할 수도 있음)
	//분할 후에 elementCount도 알아서 설정해줌(현재 노드의 count는 OctreeNodeMax+1
	void DevideOctreeNode(uint32 CurrentNodeIndex, const FAABB& Bound)
	{

		uint32 OriginSize = OctreeNodes.Num();
		//8개 추가 resize + initialize
		OctreeNodes.SetNum(OctreeNodes.Num() + 8);
		FOctreeNode& CurrentNode = OctreeNodes[CurrentNodeIndex];

		for (int Index = 0; Index < 8; Index++)
		{
			OctreeNodes[OriginSize + Index].AABB = CalculateChildAABB(CurrentNode.AABB, Index);
			OctreeNodes[OriginSize + Index].ParentIndex = CurrentNodeIndex;
			OctreeNodes[OriginSize + Index].Depth = CurrentNode.Depth + 1;
		}

		TArray<T*> ElementListToReInsert;
		//미리 capacity 확장, 초기화 X, 사이즈가 OctreeNodeMax를 초과하게 되면 분할하므로 OctreeNodeMax+1로 미리 확장
		ElementListToReInsert.reserve(OctreeNodeMax+1);
		CollectElementsFrom(CurrentNode, ElementListToReInsert);

		//다시 Octree에 element들을 insert할거라서 초기화해줌
		CurrentNode.TemporalElements.clear();
		CurrentNode.ChildStartIndex = OriginSize;
		CurrentNode.ElementCount = 0;

		//다시 insert
		for (int Index = 0; Index < ElementListToReInsert.Num(); Index++)
		{
			AddElement(ElementListToReInsert[Index], CurrentNodeIndex);
		}


	}

	void CollectElementsFrom(FOctreeNode& CurrentNode, TArray<T*>& ElementList)
	{
		int ReInsertIndex = 0;
		//Elements리스트에 있는 것들 추가, TemporalElement에 있는 것들 추가
		for (int ReInsertIndex = 0; ReInsertIndex < CurrentNode.ElementCount; ReInsertIndex++)
		{
			ElementList.Add( Elements[CurrentNode.ElementStartIndex + ReInsertIndex]);
		}
		ElementList.Append(CurrentNode.TemporalElements);
	}

	FAABB CalculateChildAABB(const FAABB& ParentBound, uint32 Index)
	{
		//Index XZY -> 000 뒤 아래 왼쪽, 001 뒤 아래 오른쪽, 010 뒤 위 왼쪽, 100 앞 아래 왼쪽
		FVector Center = ParentBound.GetCenter();
		FAABB Bound{};
		if (Index & 1)	//오른쪽
		{
			Bound.Min.Y = Center.Y;
			Bound.Max.Y = ParentBound.Max.Y;
		}
		else	//왼쪽
		{
			Bound.Min.Y = ParentBound.Min.Y;
			Bound.Max.Y = Center.Y;
		}

		if (Index & 2)	//위
		{
			Bound.Min.Z = Center.Z;
			Bound.Max.Z = ParentBound.Max.Z;
		}
		else
		{
			Bound.Min.Z = ParentBound.Min.Z;
			Bound.Max.Z = Center.Z;
		}

		if (Index & 4) //앞
		{
			Bound.Min.X = Center.X;
			Bound.Max.X = ParentBound.Max.X;
		}
		else
		{
			Bound.Min.X = ParentBound.Min.X;
			Bound.Max.X = Center.X;
		}

		return Bound;
	}

	mutable TArray<FOctreeNode> OctreeNodes;

	mutable TArray<T*> Elements;

	TArray<T*> ElementsOutsideOctree;

	mutable bool bIsElementsDirty = true;

	int TemporalElementNum = 0;
	//하나의 노드에 들어갈 수 있는 최대 객체 수
	const int OctreeNodeMax = 32;
	//이론적으로 오브젝트가 완벽히 균등하게 분포해 있으면 50,000/32 = 1562개의 리프노드만 있어도 되지만
	//오브젝트가 한 공간에 많이 뭉쳐있는 경우가 분명히 생기고 그런 일이 많이 일어나기 때문에 훨씬 크게 잡아야 한다고 함
	//테스트하면서 조절하면 됨.
	const int MaxDepth = 8;
	//TemporalElement가 100개 넘으면 재정렬
	const int MaxTemporalElementNum = 100;
};
