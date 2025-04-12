#pragma once
#include "Components/StaticMeshComponent.h"
class UProceduralMeshComponent : public UStaticMeshComponent
{
    DECLARE_CLASS(UProceduralMeshComponent, UStaticMeshComponent)

public:
    UProceduralMeshComponent() = default;

    virtual void TickComponent(float DeltaTime) override;

};
