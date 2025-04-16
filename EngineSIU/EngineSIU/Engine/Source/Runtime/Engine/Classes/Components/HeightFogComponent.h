#pragma once
#include "PrimitiveComponent.h"
#include <Math/Color.h>

class UHeightFogComponent : public UPrimitiveComponent
{
    DECLARE_CLASS(UHeightFogComponent, UPrimitiveComponent)
private:
    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;
    float FogMaxOpacity;
    FLinearColor FogInscatteringColor;

public:
    UHeightFogComponent(float Density = 0.5f, float HeightFalloff = 0.001f, float StartDist = 0.0f, float CutoffDist = 0.0f, float MaxOpacity = 0.75f);

    float GetFogDensity() { return FogDensity; }
    float GetFogHeightFalloff() { return FogHeightFalloff; }
    float GetStartDistance() { return StartDistance; }
    float GetFogCutoffDistance() { return FogCutoffDistance; }
    float GetFogMaxOpacity() { return FogMaxOpacity; }
    FLinearColor GetFogColor() { return FogInscatteringColor; }
    
    void SetFogDensity(float value);
    void SetFogHeightFalloff(float value);
    void SetStartDistance(float value);
    void SetFogCutoffDistance(float value);
    void SetFogMaxOpacity(float value);
    void SetFogColor(FLinearColor color);

    virtual UObject* Duplicate(UObject* InOuter) override;
};
