#pragma once
#include "Runtime/Engine/Classes/Actors/Lights/LightActor.h"

class UGizmoArrowComponent;

class ADirectionalLightActor : public ALight {
    DECLARE_CLASS(ADirectionalLightActor, ALight)
public:
    ADirectionalLightActor();
    virtual ~ADirectionalLightActor();
private:
    UGizmoArrowComponent* GizmoArrowComponent;
};
