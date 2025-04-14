#pragma once
#include "Runtime/Engine/Classes/Actors/Lights/LightActor.h"

class ASpotLightActor : public ALight {
    DECLARE_CLASS(ASpotLightActor, ALight)
public:
    ASpotLightActor();
    virtual ~ASpotLightActor();
};
