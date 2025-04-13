#include "PointLightComponent.h"
#include "Editor/PropertyEditor/PropertyEditorPanel.h"
#include "Runtime/Launch/EngineLoop.h"
#include <Engine/EditorEngine.h>

UPointLightComponent::UPointLightComponent()
{
    Light.Type = ELightType::POINT_LIGHT;
    Radius = 1.0f;
    GizmoColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    Segements = 32;
}

UPointLightComponent::~UPointLightComponent()
{
}

void UPointLightComponent::DrawGizmo()
{

    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (GetOwner() == Engine->GetSelectedActor()) {
        FEngineLoop::PrimitiveDrawBatch.AddSpehreToBatch(RelativeLocation, Light.AttRadius, FVector4(1.0f, 1.0f, 1.0f, 1.0f), 32);
    }
}
