#include "StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"


AStaticMeshActor::AStaticMeshActor()
{
    StaticMeshComponent = AddComponent<UStaticMeshComponent>();
    RootComponent = StaticMeshComponent;
}

UStaticMeshComponent* AStaticMeshActor::GetStaticMeshComponent() const
{
    return StaticMeshComponent;
}

