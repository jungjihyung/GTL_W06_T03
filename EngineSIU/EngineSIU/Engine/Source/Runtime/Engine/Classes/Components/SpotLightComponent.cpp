#include "SpotLightComponent.h"
#include <Engine/EditorEngine.h>
#include <Actors/Lights/SpotlightActor.h>
#include <Math/JungleMath.h>
USpotLightComponent::USpotLightComponent()
{
    Light.Type = ELightType::SPOT_LIGHT;
}

USpotLightComponent::~USpotLightComponent()
{
}

FVector USpotLightComponent::GetDirection()
{
    return Light.Direction;
}

void USpotLightComponent::SetDirection(const FVector& dir)
{
    Light.Direction = dir;
}

void USpotLightComponent::DrawGizmo()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    //ASpotLightActor* spotlightActor = Cast<ASpotLightActor>(GetOwner());
    FMatrix Model = JungleMath::CreateModelMatrix(GetWorldLocation(), GetWorldRotation(), GetWorldScale3D());
    if (GetOwner() == Engine->GetSelectedActor()) {
        // FIXME : 반지름, 높이 lightdata에서 넘기게

        FEngineLoop::PrimitiveDrawBatch.AddConeToBatch(GetWorldLocation(), 10.0f, Light.OuterConeAngle, FVector4(1.0f, 1.0f, 1.0f, 1.0f), Model);
    }

}
