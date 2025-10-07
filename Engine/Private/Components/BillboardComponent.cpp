#include "pch.h"
#include "Components/BillboardComponent.h"
#include "Level/Level.h"
#include "Public/Manager/Resource/ResourceManager.h"

IMPLEMENT_CLASS(UBillboardComponent, UPrimitiveComponent)

UBillboardComponent::UBillboardComponent()
{
    ComponentType = EComponentType::Billboard;
    bIsScreenSizeScaled = true;
    ScreenSize = 0.05f; // 화면 높이의 5%로 보이게 (현재는 데이터만 유지)
    U = V = 0.f;
    UL = VL = 1.f;
    SpriteColor = FVector4(1, 1, 1, 1);
}

UBillboardComponent::~UBillboardComponent()
{
    if (SpriteSRV)
    {
        SpriteSRV->Release();
        SpriteSRV = nullptr;
    }
}

void UBillboardComponent::SetScreenSize(float InScreenSize)
{

    ScreenSize = clamp(InScreenSize, 0.0f, 1.0f);
    MarkAsDirty();
}

void UBillboardComponent::SetUV(float InU, float InV, float InUL, float InVL)
{
    U = InU; V = InV; UL = InUL; VL = InVL;
    MarkAsDirty();
}

void UBillboardComponent::GetUV(float& OutU, float& OutV, float& OutUL, float& OutVL) const
{
    OutU = U; OutV = V; OutUL = UL; OutVL = VL;
}

void UBillboardComponent::ComputeWorldSize(float& OutWidth, float& OutHeight) const
{
    // 텍스처 시스템 미통합 상태: UV 비율로 종횡비 근사
    const float Aspect = (VL != 0.0f) ? (UL / VL) : 1.0f;

    // UI에서 조정하는 컴포넌트 스케일을 크기에 반영
    // X 스케일 -> 너비, Y 스케일 -> 높이로 매핑
    const FVector S = GetRelativeScale3D();
    const float SX = std::max(0.01f, S.X);
    const float SY = std::max(0.01f, S.Y);

    OutHeight = SY;            // 높이
    OutWidth  = SX * Aspect;   // 종횡비 반영한 너비
}

FAABB UBillboardComponent::GetWorldBounds() const
{
    float W = 0, H = 0; ComputeWorldSize(W, H);
    const FVector C = GetWorldLocation();
    const FVector Extent(W * 0.5f, H * 0.5f, 0.01f); // 얇은 판으로 처리
    return FAABB(C - Extent, C + Extent);
}

bool UBillboardComponent::IsRayCollided(const FRay& /*WorldRay*/, float& /*ShortestDistance*/) const
{
    // 아직 빌보드 충돌은 지원하지 않음
    return false;
}

void UBillboardComponent::AddToRenderList(ULevel* Level)
{
    if (Level)
    {
        Level->AddBillboardComponentToRender(this);
    }
}

void UBillboardComponent::SetSpriteFromPath(const FString& InPath)
{
    SpritePath = InPath;
    UResourceManager& RM = UResourceManager::GetInstance();
    if (SpriteSRV)
    {
        SpriteSRV->Release();
        SpriteSRV = nullptr;
    }
    SpriteSRV = RM.LoadTexture(SpritePath);
    MarkAsDirty();
}
