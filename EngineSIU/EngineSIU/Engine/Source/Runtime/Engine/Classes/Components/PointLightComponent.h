#pragma once
#include "LightComponent.h"

class UPointLightComponent :public ULightComponentBase
{

    DECLARE_CLASS(UPointLightComponent, ULightComponentBase)
public:
    UPointLightComponent();
    virtual ~UPointLightComponent() override;

    void DrawGizmo() override;
private:
    float Radius = 1.0f;
    FVector4 GizmoColor;
    int Segements;
};
