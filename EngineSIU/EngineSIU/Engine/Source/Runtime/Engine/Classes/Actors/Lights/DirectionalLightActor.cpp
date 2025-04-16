#include "DirectionalLightActor.h"
#include "Runtime/Engine/Classes/Components/BillboardComponent.h"
#include "Engine/FLoaderOBJ.h"
#include "Components/DirectionalLightComponent.h"
#include "BaseGizmos/GizmoArrowComponent.h"

ADirectionalLightActor::ADirectionalLightActor()
{
    LightComponent = AddComponent<UDirectionalLightComponent>();
    BillboardComponent = AddComponent<UBillboardComponent>();
    RootComponent = LightComponent;

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/DirectionalLight_64x.png");
    BillboardComponent->SetupAttachment(RootComponent);

    LightComponent->SetIntensity(0.05f);
}

ADirectionalLightActor::~ADirectionalLightActor()
{
}
