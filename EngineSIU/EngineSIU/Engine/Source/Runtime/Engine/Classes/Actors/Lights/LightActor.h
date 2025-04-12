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
protected:
  
    UPROPERTY
    (ULightComponentBase*, LightComponent, = nullptr);

   UPROPERTY
   (UBillboardComponent*, BillboardComponent, = nullptr);
};
