#pragma once

#include "Components/PrimitiveComponent.h"

struct ID3D11ShaderResourceView;

// 현재 엔진 구조에 맞춘 간소화된 Billboard 컴포넌트
// - 전용 렌더 파이프라인이 아직 없어 렌더 제출은 수행하지 않음
// - AABB, UV, 화면비율 옵션 등 데이터 보관과 월드 바운드 반환만 제공

class ULevel;

class UBillboardComponent : public UPrimitiveComponent
{
    DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)

public:
    UBillboardComponent();
    ~UBillboardComponent() override;

    // 화면 크기 기반 스케일링 옵션
    void SetIsScreenSizeScaled(bool bInScaled) { bIsScreenSizeScaled = bInScaled; MarkAsDirty(); }
    bool IsScreenSizeScaled() const { return bIsScreenSizeScaled; }

    // 화면 높이 비율(0~1). 현재 컴포넌트 내부에서는 데이터만 유지
    void SetScreenSize(float InScreenSize);
    float GetScreenSize() const { return ScreenSize; }

    // UV 사각형(오프셋 + 크기)
    void SetUV(float InU, float InV, float InUL, float InVL);
    void GetUV(float& OutU, float& OutV, float& OutUL, float& OutVL) const;

    // 컬러 틴트(색상 저장. 렌더 통합시 사용)
    void SetColor(const FVector4& InColor) { SpriteColor = InColor; }
    const FVector4& GetColor() const { return SpriteColor; }

    // 스프라이트 텍스처 경로/리소스 관리
    void SetSpriteFromPath(const FString& InPath);
    const FString& GetSpritePath() const { return SpritePath; }
    ID3D11ShaderResourceView* GetSpriteSRV() const { return SpriteSRV; }

    // UPrimitiveComponent 인터페이스 구현
    void AddToRenderList(ULevel* Level) override;    // 현재는 렌더 큐에 올리지 않음
    bool IsRayCollided(const FRay& WorldRay, float& ShortestDistance) const override;
    FAABB GetWorldBounds() const override;

private:
    // 간단한 월드 크기 산출: 현재는 화면 스케일링 미적용 시 고정 높이 사용
    void ComputeWorldSize(float& OutWidth, float& OutHeight) const;

private:
    bool  bIsScreenSizeScaled = false; // true=화면 비율 기준 스케일링(향후 렌더 단계 적용 예정)
    float ScreenSize = 0.05f;          // 화면 높이 비율(0~1)
    float U = 0.f;                     // 좌상단 UV 오프셋
    float V = 0.f;
    float UL = 1.f;                    // UV 폭/높이(0~1)
    float VL = 1.f;

    FVector4 SpriteColor = FVector4(1, 1, 1, 1);

    // Sprite resource (선택형)
    FString SpritePath;
    ID3D11ShaderResourceView* SpriteSRV = nullptr;
};
