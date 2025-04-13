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

void ULightComponentBase::SetDiffuseColor(FLinearColor NewColor)
{
    Light.DiffuseColor = FVector(NewColor.R, NewColor.G, NewColor.B);
}

void ULightComponentBase::SetSpecularColor(FLinearColor NewColor)
{
   Light.SpecularColor = FVector(NewColor.R, NewColor.G, NewColor.B);
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

FLinearColor ULightComponentBase::GetDiffuseColor()
{
    return FLinearColor(Light.DiffuseColor.X, Light.DiffuseColor.Y, Light.DiffuseColor.Z, 1);
}

FLinearColor ULightComponentBase::GetSpecularColor()
{
    return FLinearColor(Light.SpecularColor.X, Light.SpecularColor.Y, Light.SpecularColor.Z, 1);
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

void ULightComponentBase::InitializeLight()
{  
    AABB.max = { 1.f,1.f,0.1f };
    AABB.min = { -1.f,-1.f,-0.1f };
    
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


