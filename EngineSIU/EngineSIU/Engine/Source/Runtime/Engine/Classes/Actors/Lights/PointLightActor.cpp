#include "PointLightActor.h"
#include "Runtime/Engine/Classes/Components/PointLightComponent.h"
#include "Runtime/Engine/Classes/Components/BillboardComponent.h"
APointLightActor::APointLightActor()
{
    LightComponent = AddComponent<UPointLightComponent>();
    BillboardComponent = AddComponent<UBillboardComponent>();
    RootComponent = BillboardComponent;

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/PointLight_64x.png");
    LightComponent->SetupAttachment(RootComponent);
    LightComponent->SetIntensity(600.f);
    LightComponent->SetAttenuationRadius(10.f);
    LightComponent->SetAttenuation(0.05f);
}

APointLightActor::~APointLightActor()
{
}
