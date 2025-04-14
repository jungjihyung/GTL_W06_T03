#pragma once
#include "LightComponent.h"

class UDirectionalLightComponent : public ULightComponentBase {
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponentBase)
public:
    UDirectionalLightComponent();
    ~UDirectionalLightComponent();
    FVector GetDirection();
    void SetDirection(const FVector& dir);
    void DrawGizmo() override;
};
