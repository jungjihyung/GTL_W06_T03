#pragma once
#include "LightComponent.h"

class UPointLightComponent :public ULightComponentBase
{

    DECLARE_CLASS(UPointLightComponent, ULightComponentBase)
public:
    UPointLightComponent();
    virtual ~UPointLightComponent() override;

};
