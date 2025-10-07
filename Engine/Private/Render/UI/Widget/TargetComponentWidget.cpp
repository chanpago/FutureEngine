#include "pch.h"
#include "Render/UI/Widget/TargetComponentWidget.h"

#include "Level/Level.h"
#include "Manager/Level/World.h"
#include "Core/ObjectIterator.h"
#include "Mesh/StaticMesh.h"
#include "Mesh/Material.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/BillboardComponent.h"
#include "Manager/Path/PathManager.h"
#include "Public/Manager/Resource/ResourceManager.h"
#include "Manager/UI/UIManager.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <string>

// UTF-8(char*) -> FWstring(std::wstring)
inline FWstring Utf8ToFW(const char* Utf8)
{
	if (!Utf8) return FWstring();

#ifdef _WIN32
	// 길이 질의(널 포함)
	const int lenW = MultiByteToWideChar(CP_UTF8, 0, Utf8, -1, nullptr, 0);
	if (lenW <= 0) return FWstring();

	FWstring w;
	w.resize(lenW - 1); // 널 제외한 실제 길이
	if (lenW > 1)
		MultiByteToWideChar(CP_UTF8, 0, Utf8, -1, w.data(), lenW);
	return w;
#else
	// 비윈도우: C++17에서 deprecated지만 간단 대안
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.from_bytes(Utf8);
#endif
}

IMPLEMENT_CLASS(UTargetComponentWidget, UWidget)
UTargetComponentWidget::UTargetComponentWidget()
{
}

UTargetComponentWidget::~UTargetComponentWidget() = default;


void UTargetComponentWidget::RenderComponentWidget(USceneComponent* Component)
{
	if (!Component)
	{
		return;
	}
	UpdateTransformFromComponent(Component);
	bPositionChanged |= ImGui::DragFloat3("Location", &EditLocation.X, 0.1f);
	bRotationChanged |= ImGui::DragFloat3("Rotation", &EditRotation.X, 0.1f);

	// Uniform Scale 옵션
	bool bUniformScale = Component->IsUniformScale();
	if (bUniformScale)
	{
		float UniformScale = EditScale.X;

		if (ImGui::DragFloat("Scale", &UniformScale, 0.01f, 0.01f, 10.0f))
		{
			EditScale = FVector(UniformScale, UniformScale, UniformScale);
			bScaleChanged = true;
		}
	}
	else
	{
		bScaleChanged |= ImGui::DragFloat3("Scale", &EditScale.X, 0.1f);
	}

	ImGui::Checkbox("Uniform Scale", &bUniformScale);
	Component->SetUniformScale(bUniformScale);



	//앞으로 수많은 Component들을 추가하게 될 텐데 이렇게 매번 캐스팅하고 함수를 생성하는 건 확장성이 굉장히 떨어지는 방식임
	//캐싱 데이터를 가질 컨텍스트를 만들고 그걸 인자로 받는 가상함수를 각각의 Component가 호출하는 방식으로 리팩토링 해야함
	//일단 급한대로 RTTI로 처리함
	if(Component->IsA(UStaticMeshComponent::StaticClass()))
	{
		UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(Component);
		RenderStaticMeshComponent(StaticMeshComponent);
	}
	if (Component->IsA(UBillboardComponent::StaticClass()))
	{
		RenderBillboardComponent(static_cast<UBillboardComponent*>(Component));
	}
	if (Component->IsA(UTextRenderComponent::StaticClass()))
	{
		RenderTextRenderComponent(static_cast<UTextRenderComponent*>(Component));
	}


}

void UTargetComponentWidget::RenderStaticMeshComponent(UStaticMeshComponent* Component)
{
	if (ImGui::CollapsingHeader("Static Mesh##Header"))
	{
		UpdateStaticMeshListCash();

		const UStaticMesh* CurrentStaticMesh = Component->GetStaticMesh();

		uint32 CurrentUUID = -1;
		FString PreviewString("Null");
		if (CurrentStaticMesh)
		{
			CurrentUUID = CurrentStaticMesh->GetUUID();
			PreviewString = CurrentStaticMesh->GetAssetPathFileName();
		}

		if (ImGui::BeginCombo("Static Mesh##Combo", PreviewString.c_str()))
		{

			for (int Index = 0; Index < StaticMeshList.Num(); Index++)
			{
				const uint32 StaticMeshUUID = StaticMeshList[Index]->GetUUID();
				const bool bIsSelected = (CurrentUUID == StaticMeshUUID);

				//선택되면 true, bool값이 true면 하이라이트
				if (ImGui::Selectable(StaticMeshList[Index]->GetAssetPathFileName().c_str(), bIsSelected))
				{
					if (CurrentUUID != StaticMeshUUID)
					{
						Component->SetStaticMesh(StaticMeshList[Index]);
					}
				}

				if (bIsSelected)
					ImGui::SetItemDefaultFocus();

			}
			ImGui::EndCombo();
		}
	}
	//Collapsing 헤더가 닫히는 순간부터는 추적이 안되므로 true(헤더를 여는 순간만 1회 업데이트 할 것임)
	//헤더를 연 상태에서 StaticMeshList가 업데이트 되면 반영 안 됨. 다시 열어야 함.
	bStaticMeshListDirty = true;
	if (Component->GetStaticMesh())
	{
		RenderMaterials(Component);
	}

}

void UTargetComponentWidget::RenderBillboardComponent(UBillboardComponent* Component)
{
    if (ImGui::CollapsingHeader("Sprite##Header"))
    {
        UpdateSpriteListCash();

        // 좌측 미니 프리뷰 (정사각형)
        const float PreviewSize = 72.0f;
        if (ID3D11ShaderResourceView* Srv = Component->GetSpriteSRV())
        {
            ImGui::Image((ImTextureID)Srv, ImVec2(PreviewSize, PreviewSize), ImVec2(0, 0), ImVec2(1, 1));
        }
        else
        {
            ImGui::Dummy(ImVec2(PreviewSize, PreviewSize));
        }
        ImGui::SameLine();

        // 현재 스프라이트 경로 표시 (파일명만)
        const FString& CurrentPath = Component->GetSpritePath();
        FString Preview = "<None>";
        if (!CurrentPath.empty())
        {
            Preview = filesystem::path(CurrentPath).stem().string();
        }

        if (ImGui::BeginCombo("Sprite (png)##Combo", Preview.c_str()))
        {
            for (int Index = 0; Index < SpritePathList.Num(); ++Index)
            {
                const FString& PathStr = SpritePathList[Index];
                const FString DisplayName = filesystem::path(PathStr).stem().string();
                bool bSelected = (PathStr == CurrentPath);
                if (ImGui::Selectable(DisplayName.c_str(), bSelected))
                {
                    if (!bSelected)
                    {
                        Component->SetSpriteFromPath(PathStr);
                    }
                }
                if (bSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
	bool ShowFlag = Component->IsVisible();
	ImGui::Checkbox("Render", &ShowFlag);
	Component->SetVisibility(ShowFlag);
    bSpriteListDirty = true;
}

void UTargetComponentWidget::RenderMaterials(UStaticMeshComponent* Component)
{
	if (ImGui::CollapsingHeader("Materials##Of StaticMesh"))
	{
		UpdateMaterialListCash();
		const TArray<UMaterial*>& MaterialListOfComponent = Component->GetMaterialList();
		if (!MaterialListOfComponent[0])
			return;
		for (int Index = 0; Index < MaterialListOfComponent.Num(); Index++)
		{
			const FString& CurrentMaterialName = MaterialListOfComponent[Index]->GetMaterialName();

			const uint32 CurrentUUID = MaterialListOfComponent[Index]->GetUUID();

			FString Tag = FString("Material ").append(std::to_string(Index));
			if (ImGui::BeginCombo(Tag.c_str(), CurrentMaterialName.c_str()))
			{
				//프로그램 내의 모든 Material에 대한 Index
				for (int WIndex = 0; WIndex < MaterialList.Num(); WIndex++)
				{
					const uint32 MaterialUUID = MaterialList[WIndex]->GetUUID();
					const bool bIsSelected = (CurrentUUID == MaterialUUID);

					//선택되면 true, bool값이 true면 하이라이트
					const FString& MaterialName = MaterialList[WIndex]->GetMaterialName();
					if (MaterialName.empty())
						continue;
					if (ImGui::Selectable(MaterialName.c_str(), bIsSelected))
					{
						if (CurrentUUID != MaterialUUID)
						{
							Component->SetMaterial(MaterialList[WIndex], Index);
						}
					}

					if (bIsSelected)
						ImGui::SetItemDefaultFocus();

				}
				ImGui::EndCombo();
			}
		}
	}
	bool ShowFlag = Component->IsVisible();
	ImGui::Checkbox("Render", &ShowFlag);
	Component->SetVisibility(ShowFlag);
	//Collapsing 헤더가 닫히는 순간부터는 추적이 안되므로 true(헤더를 여는 순간만 1회 업데이트 할 것임)
	//헤더를 연 상태에서 MaterialList가 업데이트 되면 반영 안 됨. 다시 열어야 함.
	bMaterialListDirty = true;
}

void UTargetComponentWidget::RenderTextRenderComponent(UTextRenderComponent* Component)
{
	ImGui::Text("텍스트");
	if (ImGui::InputText("##텍스트", TextRenderBuffer, sizeof(TextRenderBuffer)))
	{
		Component->SetText(Utf8ToFW(TextRenderBuffer));
	}
	
	bool ShowFlag = Component->IsVisible();
	ImGui::Checkbox("Render", &ShowFlag);
	Component->SetVisibility(ShowFlag);
}

void UTargetComponentWidget::UpdateMaterialListCash()
{
	if (bMaterialListDirty)
	{
		MaterialList.clear();
		for (TObjectIterator<UMaterial> It; It; ++It)
		{
			UMaterial* Material = *It;
			if (Material)
			{
				MaterialList.push_back(Material);
			}
		}
	}
	bMaterialListDirty = false;
}

void UTargetComponentWidget::UpdateStaticMeshListCash()
{

	if (bStaticMeshListDirty)
	{
		StaticMeshList.clear();
		for (TObjectIterator<UStaticMesh> It; It; ++It)
		{
			UStaticMesh* StaticMesh = *It;
			if (StaticMesh)
			{
				StaticMeshList.push_back(StaticMesh);
			}
		}
	}
	bStaticMeshListDirty = false;
}

void UTargetComponentWidget::UpdateSpriteListCash()
{
    if (!bSpriteListDirty)
        return;

    SpritePathList.clear();
    const UPathManager& PM = UPathManager::GetInstance();
    const auto& TexDir = PM.GetTexturePath();
    if (exists(TexDir))
    {
        for (const auto& entry : filesystem::directory_iterator(TexDir))
        {
            if (!entry.is_regular_file()) continue;
            const auto ext = entry.path().extension().string();
            FString lower = ext; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == ".png")
            {
                SpritePathList.Add(entry.path().string());
            }
        }
    }
    bSpriteListDirty = false;
}


void UTargetComponentWidget::UpdateTransformFromComponent(USceneComponent* Component)
{
	if (!Component)
	{
		return;
	}
	else
	{
		EditLocation = Component->GetRelativeLocation();
		EditRotation = Component->GetRelativeRotation();
		EditScale = Component->GetRelativeScale3D();
	}
}


void UTargetComponentWidget::ApplyTransformToComponent(USceneComponent* Component) const
{
	if (bPositionChanged || bRotationChanged || bScaleChanged)
	{
		if (!Component)
		{
			return;
		}
		else
		{
			Component->SetRelativeLocation(EditLocation);
			Component->SetRelativeRotation(EditRotation);
			Component->SetRelativeScale3D(EditScale);
		}
	}
}
