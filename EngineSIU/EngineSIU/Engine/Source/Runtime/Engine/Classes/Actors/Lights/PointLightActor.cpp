#include "PointLightActor.h"
#include "Runtime/Engine/Classes/Components/PointLightComponent.h"
#include "Runtime/Engine/Classes/Components/BillboardComponent.h"
APointLightActor::APointLightActor()
{
    LightComponent = AddComponent<UPointLightComponent>();
    BillboardComponent = AddComponent<UBillboardComponent>();

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/PointLight_64x.png");
    LightComponent->SetupAttachment(RootComponent);


}

APointLightActor::~APointLightActor()
{
}
