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
#include <Components/DirectionalLightComponent.h>
#include <Actors/Lights/LightActor.h>
#include "Runtime/Engine/Classes/Components/BillboardComponent.h"

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

    CreateLightStructuredBuffer();
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
            else if (UDirectionalLightComponent* DirLight = Cast<UDirectionalLightComponent>(iter)) {
                DirLights.Add(DirLight);
            }
        }
    }
}

void FUpdateLightBufferPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FLightBuffer LightBufferData = {};
    FLight Lights[MAX_LIGHTS] = {};

    int LightCount = 0;

    LightBufferData.GlobalAmbientLight = FVector4(0.2f, 0.2f, 0.2f, 1.f);
    for (auto Light : PointLights)
    {
        if (LightCount < MAX_LIGHTS)
        {
            //FIXME : 컴포넌트의 자식 컴포넌트에 위치 변경 값 반영 안되어서 임시로 설정. 추후 변경 필요.
            ALight* lightActor = Cast<ALight>(Light->GetOwner());
            lightActor->GetBillboardComponent()->SetRelativeLocation(Light->GetWorldLocation());

            Lights[LightCount] = Light->GetLightInfo();
            Lights[LightCount].Position = Light->GetWorldLocation();

            LightCount++;
            Light->DrawGizmo();


            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Light->GetBoundingBox(), Light->GetWorldLocation(), Light->GetWorldMatrix());

        }
    }

    for (auto Light : SpotLights)
    {
        if (LightCount < MAX_LIGHTS)
        {

            //FIXME : 컴포넌트의 자식 컴포넌트에 위치 변경 값 반영 안되어서 임시로 설정. 추후 변경 필요.
            ALight* lightActor = Cast<ALight>(Light->GetOwner());
            lightActor->GetBillboardComponent()->SetRelativeLocation(Light->GetWorldLocation());

            Lights[LightCount] = Light->GetLightInfo();
            Lights[LightCount].Position = Light->GetWorldLocation();
            Lights[LightCount].Direction = Light->GetForwardVector();
            Lights[LightCount].Type = ELightType::SPOT_LIGHT;

            LightCount++;
            Light->DrawGizmo();

        }
    }

    for (auto Light : DirLights) {
        if (LightCount < MAX_LIGHTS) {

            //FIXME : 컴포넌트의 자식 컴포넌트에 위치 변경 값 반영 안되어서 임시로 설정. 추후 변경 필요.
            ALight* lightActor = Cast<ALight>(Light->GetOwner());
            lightActor->GetBillboardComponent()->SetRelativeLocation(Light->GetWorldLocation());

            //  FIXING : Direction 확인하고 고치기
            Lights[LightCount] = Light->GetLightInfo();
            Lights[LightCount].Position = Light->GetWorldLocation();
            Lights[LightCount].Direction = Light->GetForwardVector();
            Lights[LightCount].Type = ELightType::DIR_LIGHT;

            LightCount++;
            Light->DrawGizmo();
        }
    }
    LightBufferData.nLights = LightCount;

    BufferManager->UpdateConstantBuffer(TEXT("FLightBuffer"), LightBufferData);

    // Update light buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Graphics->DeviceContext->Map(Graphics->LightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, Lights, sizeof(FLight) * MAX_LIGHTS);

        Graphics->DeviceContext->Unmap(Graphics->LightBuffer, 0);
    }
}

void FUpdateLightBufferPass::ClearRenderArr()
{
    PointLights.Empty();
    SpotLights.Empty();
}

void FUpdateLightBufferPass::UpdateLightBuffer(FLight Light) const
{

}

void FUpdateLightBufferPass::CreateLightStructuredBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(FLight) * MAX_LIGHTS;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(FLight);

    HRESULT hr = Graphics->Device->CreateBuffer(&desc, nullptr, &Graphics->LightBuffer);
    if (FAILED(hr))
    {
        // Handle error
        MessageBox(nullptr, L"Failed to create LightBuffer!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // create srv
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = MAX_LIGHTS;

    hr = Graphics->Device->CreateShaderResourceView(Graphics->LightBuffer, &srvDesc, &Graphics->LightBufferSRV);
    if (FAILED(hr))
    {
        // Handle error
        MessageBox(nullptr, L"Failed to create LightBufferSRV!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }
}
