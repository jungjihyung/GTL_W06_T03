#include "LightComponent.h"
#include "BillboardComponent.h"
#include "UObject/Casts.h"

ULightComponentBase::ULightComponentBase()
{
    // FString name = "SpotLight";
    // SetName(name);
    InitializeLight();
}

ULightComponentBase::~ULightComponentBase()
{
  
}

UObject* ULightComponentBase::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->Light = Light;

    return NewComponent;
}

void ULightComponentBase::SetBaseColor(FLinearColor NewColor)
{
    Light.BaseColor = FVector(NewColor.R, NewColor.G, NewColor.B);
}


void ULightComponentBase::SetAttenuation(float Attenuation)
{
    Light.Attenuation = Attenuation;
}

void ULightComponentBase::SetAttenuationRadius(float AttenuationRadius)
{
    Light.AttRadius = AttenuationRadius;
}

void ULightComponentBase::SetIntensity(float Intensity)
{
    Light.Intensity = Intensity;
}

void ULightComponentBase::SetFalloff(float fallOff)
{
    Light.Falloff = fallOff;
}

void ULightComponentBase::SetInnerConeAngle(float InnerAngle)
{
    Light.InnerConeAngle = InnerAngle;
}

void ULightComponentBase::SetOuterConeAngle(float OuterAngle)
{
    Light.OuterConeAngle = OuterAngle;
}

FLinearColor ULightComponentBase::GetBaseColor()
{
    return FLinearColor(Light.BaseColor.X, Light.BaseColor.Y, Light.BaseColor.Z, 1);
}

float ULightComponentBase::GetAttenuation()
{
    return Light.Attenuation;
}

float ULightComponentBase::GetAttenuationRadius()
{
    return Light.AttRadius;
}

float ULightComponentBase::GetFalloff()
{
    return Light.Falloff;
}

float ULightComponentBase::GetInnerConeAngle()
{
    return Light.InnerConeAngle;
}

float ULightComponentBase::GetOuterConeAngle()
{
    return Light.OuterConeAngle;
}

void ULightComponentBase::InitializeLight()
{  
    AABB.max = { 1.f,1.f,1.f };
    AABB.min = { -1.f,-1.f,-1.f };
    
    Light = FLight();
    Light.Enabled = 1;
}

void ULightComponentBase::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

int ULightComponentBase::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    bool res = AABB.Intersect(rayOrigin, rayDirection, pfNearHitDistance);
    return res;
}

void ULightComponentBase::DrawGizmo()
{
}


