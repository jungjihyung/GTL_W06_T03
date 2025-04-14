#pragma once
#include "Runtime/Engine/Classes/Actors/Lights/LightActor.h"
class APointLightActor : public ALight
{
    DECLARE_CLASS(APointLightActor, ALight)
public:
    APointLightActor();
    virtual ~APointLightActor();
};
