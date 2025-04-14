#pragma once
#include "GameFramework/Actor.h"

class ULightComponentBase;
class UBillboardComponent;

class ALight :public AActor
{
    DECLARE_CLASS(ALight, AActor)
public:
    ALight();
    virtual ~ALight();

    UBillboardComponent* GetBillboardComponent() { return BillboardComponent; }
protected:
  
    UPROPERTY
    (ULightComponentBase*, LightComponent, = nullptr);

   UPROPERTY
   (UBillboardComponent*, BillboardComponent, = nullptr);
};
