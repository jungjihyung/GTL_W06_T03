#include "DirectionalLightActor.h"
#include "Runtime/Engine/Classes/Components/BillboardComponent.h"
#include "Engine/FLoaderOBJ.h"
#include "Components/DirectionalLightComponent.h"
#include "BaseGizmos/GizmoArrowComponent.h"

ADirectionalLightActor::ADirectionalLightActor()
{
    LightComponent = AddComponent<UDirectionalLightComponent>();
    BillboardComponent = AddComponent<UBillboardComponent>();
    RootComponent = BillboardComponent;

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/DirectionalLight_64x.png");
    LightComponent->SetupAttachment(RootComponent);

    GizmoArrowComponent = AddComponent<UGizmoArrowComponent>();
    GizmoArrowComponent->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Assets/GizmoTranslationY.obj"));
    GizmoArrowComponent->SetupAttachment(RootComponent);
}

ADirectionalLightActor::~ADirectionalLightActor()
{
}
