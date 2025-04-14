#include "LightActor.h"
#include "Components/PointLightComponent.h"
#include "Components/BillboardComponent.h"
ALight::ALight()
{
    RootComponent = BillboardComponent; 
}

ALight::~ALight()
{
}
