#include "SpotlightActor.h"
#include "Engine/Source/Runtime/Engine/Classes/Components/SpotLightComponent.h"
#include "Engine/Source/Runtime/Engine/Classes/Components/BillboardComponent.h"
ASpotLightActor::ASpotLightActor()
{
    LightComponent = AddComponent<USpotLightComponent>();
    BillboardComponent = AddComponent<UBillboardComponent>();
    RootComponent = LightComponent;

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/SpotLight_64x.png");
    BillboardComponent->SetupAttachment(RootComponent);
}

ASpotLightActor::~ASpotLightActor()
{
}
