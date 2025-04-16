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

    if (GetOwner() == Engine->GetSelectedActor()) {
        // 회전 반전 (쿼터니언 Inverse 사용)
        FQuat InvertedRotation = GetWorldRotation().ToQuaternion().Inverse();
        FMatrix Model = JungleMath::CreateModelMatrix(GetWorldLocation(), InvertedRotation, {1, 1, 1});
        FEngineLoop::PrimitiveDrawBatch.AddConeToBatch(GetWorldLocation(), 10.0f, Light.OuterConeAngle, FVector4(1.0f, 1.0f, 1.0f, 1.0f), Model);
    }

}
