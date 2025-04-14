#pragma once

#define _TCHAR_DEFINED
#include <wrl.h>
#include "PrimitiveComponent.h"


class UBillboardComponent : public UPrimitiveComponent
{
    DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)

public:
    UBillboardComponent();
    virtual ~UBillboardComponent();
    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(
        FVector& rayOrigin,
        FVector& rayDirection,
        float& pfNearHitDistance
    ) override;

    virtual void SetTexture(const FWString& _fileName);
    void SetUUIDParent(USceneComponent* _parent);
    void SetTintColor(FLinearColor color);
    FMatrix CreateBillboardMatrix() const;
    FString GetBufferKey();

    float finalIndexU = 0.0f;
    float finalIndexV = 0.0f;
    std::shared_ptr<FTexture> Texture;
    FString BufferKey = TEXT("default");
    FLinearColor TintColor;
protected:
    USceneComponent* m_parent = nullptr;

    // NDC 픽킹을 위한 내부 함수 : quadVertices는 월드 공간 정점 배열
    bool CheckPickingOnNDC(const TArray<FVector>& quadVertices, float& hitDistance) const;
};
