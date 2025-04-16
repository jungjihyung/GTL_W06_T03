#pragma once
#include "Runtime/Engine/Classes/Actors/Lights/LightActor.h"

class UStaticMeshComponent;

class ADirectionalLightActor : public ALight {
    DECLARE_CLASS(ADirectionalLightActor, ALight)
public:
    ADirectionalLightActor();
    virtual ~ADirectionalLightActor();

    UStaticMeshComponent* GetGizmoComponent();

private:
    UStaticMeshComponent* GizmoComponent;
};
