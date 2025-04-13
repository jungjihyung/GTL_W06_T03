#include "Define.h"
#include "UObject/Casts.h"
#include "UpdateLightBufferPass.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Math/JungleMath.h"
#include "Engine/EditorEngine.h"
#include "World/World.h"
#include "EngineLoop.h"
#include "GameFramework/Actor.h"

#include "UObject/UObjectIterator.h"

//------------------------------------------------------------------------------
// 생성자/소멸자
//------------------------------------------------------------------------------
FUpdateLightBufferPass::FUpdateLightBufferPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FUpdateLightBufferPass::~FUpdateLightBufferPass()
{
}

void FUpdateLightBufferPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;
}

void FUpdateLightBufferPass::PrepareRender()
{
    for (const auto iter : TObjectRange<ULightComponentBase>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(iter))
            {
                PointLights.Add(PointLight);
            }
            else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(iter))
            {
                SpotLights.Add(SpotLight);
            }
        }
    }
}

void FUpdateLightBufferPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FLightBuffer LightBufferData = {};
    int LightCount = 0;

    LightBufferData.GlobalAmbientLight = FVector4(0.1f, 0.1f, 0.1f, 1.f);
    for (auto Light : PointLights)
    {
        if (LightCount < MAX_LIGHTS)
        {
            LightBufferData.gLights[LightCount] = Light->GetLightInfo();
            LightBufferData.gLights[LightCount].Position = Light->GetWorldLocation();

            LightCount++;
            Light->DrawGizmo();
        }
    }

    for (auto Light : SpotLights)
    {
        if (LightCount < MAX_LIGHTS)
        {
            //// 월드 변환 행렬 계산 (스케일 1로 가정)
            //FMatrix Model = JungleMath::CreateModelMatrix(Light->GetWorldLocation(), Light->GetWorldRotation(), { 1, 1, 1 });

            //FEngineLoop::PrimitiveDrawBatch.AddConeToBatch(Light->GetWorldLocation(), 100, Light->GetRange(), 140, {1,1,1,1}, Model);

            //FEngineLoop::PrimitiveDrawBatch.AddOBBToBatch(Light->GetBoundingBox(), Light->GetWorldLocation(), Model);
            LightBufferData.gLights[LightCount] = Light->GetLightInfo();
            LightBufferData.gLights[LightCount].Position = Light->GetWorldLocation();
            LightBufferData.gLights[LightCount].Direction = Light->GetForwardVector();
            LightBufferData.gLights[LightCount].Type = ELightType::SPOT_LIGHT;

            LightCount++;
            
        }
    }
    LightBufferData.nLights = LightCount;

    BufferManager->UpdateConstantBuffer(TEXT("FLightBuffer"), LightBufferData);
}

void FUpdateLightBufferPass::ClearRenderArr()
{
    PointLights.Empty();
    SpotLights.Empty();
}

void FUpdateLightBufferPass::UpdateLightBuffer(FLight Light) const
{

}
