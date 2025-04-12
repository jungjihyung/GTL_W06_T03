#include "FireballActor.h"
#include "Engine/FLoaderOBJ.h"

#include "Components/PointLightComponent.h"

#include "Components/ProjectileMovementComponent.h"

#include "Components/SphereComp.h"

AFireballActor::AFireballActor()
{
    FManagerOBJ::CreateStaticMesh("Contents/Sphere.obj");


    SphereComp = AddComponent<USphereComp>();
    
    SphereComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Contents/Sphere.obj"));
  
    PointLightComponent = AddComponent<UPointLightComponent>();
    

    PointLightComponent->SetAttenuation(0.5f);
    
    PointLightComponent->SetBaseColor(FLinearColor::Red);
    
    ProjectileMovementComponent = AddComponent<UProjectileMovementComponent>();
    PointLightComponent->AttachToComponent(RootComponent);

    ProjectileMovementComponent->SetGravity(0);
    ProjectileMovementComponent->SetVelocity(FVector(100, 0, 0));
    ProjectileMovementComponent->SetInitialSpeed(100);
    ProjectileMovementComponent->SetMaxSpeed(100);
    ProjectileMovementComponent->SetLifetime(10);
}

AFireballActor::~AFireballActor()
{
}

void AFireballActor::BeginPlay()
{
}
