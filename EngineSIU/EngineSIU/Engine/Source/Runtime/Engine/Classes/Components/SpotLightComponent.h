#pragma once
#include "LightComponent.h"

class USpotLightComponent :public ULightComponentBase
{

    DECLARE_CLASS(USpotLightComponent, ULightComponentBase)
public:
    USpotLightComponent();
    ~USpotLightComponent();
    FVector GetDirection();
    void SetDirection(const FVector& dir);

    void DrawGizmo() override;
};

