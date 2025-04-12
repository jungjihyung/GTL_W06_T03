#pragma once
#include "PrimitiveComponent.h"
#include "Define.h"
class UBillboardComponent;

class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() override;
    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance) override;
    void InitializeLight();
    
    void SetDiffuseColor(FLinearColor NewColor);
    void SetSpecularColor(FLinearColor NewColor);
    void SetAttenuation(float Attenuation);
    void SetAttenuationRadius(float AttenuationRadius);
    void SetIntensity(float Intensity);
    void SetFalloff(float fallOff);

    FLinearColor GetDiffuseColor();
    FLinearColor GetSpecularColor();
    float GetAttenuation();
    float GetAttenuationRadius();
    float GetFalloff();
    FLight GetLightInfo() const { return Light; };
protected:

    FBoundingBox AABB;
   
    FLight Light;

public:
    FBoundingBox GetBoundingBox() const {return AABB;}
    
    float GetIntensity() const {return Light.Intensity;}
    
};
