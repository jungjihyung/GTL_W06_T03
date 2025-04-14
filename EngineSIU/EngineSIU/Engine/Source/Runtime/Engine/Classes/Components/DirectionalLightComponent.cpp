#include "DirectionalLightComponent.h"
#include <Engine/EditorEngine.h>

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
    if (GetOwner() == Engine->GetSelectedActor()) {
        // TODO : 기즈모 렌더링 로직 추가
    }
}
