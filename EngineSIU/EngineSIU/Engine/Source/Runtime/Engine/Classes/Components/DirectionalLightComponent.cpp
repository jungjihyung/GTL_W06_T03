#include "DirectionalLightComponent.h"
#include <Engine/EditorEngine.h>
#include <Math/JungleMath.h>
#include <Actors/Lights/DirectionalLightActor.h>
#include "Runtime/InteractiveToolsFramework/BaseGizmos/GizmoArrowComponent.h"
#include <Engine/FLoaderOBJ.h>
UDirectionalLightComponent::UDirectionalLightComponent()
{
    Light.Type = ELightType::DIR_LIGHT;
}

UDirectionalLightComponent::~UDirectionalLightComponent() 
{
}

FVector UDirectionalLightComponent::GetDirection()
{
    return Light.Direction;
}

void UDirectionalLightComponent::SetDirection(const FVector& dir)
{
    Light.Direction = dir;
}

void UDirectionalLightComponent::DrawGizmo()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    ADirectionalLightActor* dirActor = Cast<ADirectionalLightActor>(GetOwner());
    if (GetOwner() == Engine->GetSelectedActor()) {
        FMatrix Model = JungleMath::CreateModelMatrix(FVector(0, 0, 0), GetWorldRotation().ToQuaternion(), { 1, 1, 1 });
        FEngineLoop::PrimitiveDrawBatch.AddConeToBatch(GetWorldLocation() + GetForwardVector(), -1.0f, 20.0f, FVector4(0.4f, 1.0f, 0.4f, 1.0f), GetRotationMatrix());
        dirActor->GetGizmoComponent()->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Assets/GizmoTranslationY.obj"));
    }
}
