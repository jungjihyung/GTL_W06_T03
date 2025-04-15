#include "GizmoRenderPass.h"

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "RendererHelpers.h"
#include "Math/JungleMath.h"

#include "World/World.h"

#include "Actors/Player.h"

#include "BaseGizmos/GizmoBaseComponent.h"
#include "BaseGizmos/TransformGizmo.h"

#include "UnrealEd/EditorViewportClient.h"

#include "PropertyEditor/ShowFlags.h"

#include "EngineLoop.h"

#include "UObject/ObjectTypes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/EditorEngine.h"
#include "D3D11RHI/HotReload/ShaderHashUtils.h"


// 생성자/소멸자
FGizmoRenderPass::FGizmoRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FGizmoRenderPass::~FGizmoRenderPass()
{
    ReleaseShader();
    if (ShaderManager)
    {
        delete ShaderManager;
        ShaderManager = nullptr;
    }
}

void FGizmoRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;
    CreateShader();
}

void FGizmoRenderPass::CreateShader()
{

    Stride = sizeof(FStaticMeshVertex);

    GizmoVertexShaderKey = ShaderHashUtils::ComputeHashKey(ShaderCompileInfo(L"Shaders/StaticMeshVertexShader.hlsl", "mainVS", DefineUnLit));
    GizmoPixelShaderKey = ShaderHashUtils::ComputeHashKey(ShaderCompileInfo(L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefineUnLit));

    VertexShader = ShaderManager->GetVertexShaderByKey(GizmoVertexShaderKey);
    PixelShader = ShaderManager->GetPixelShaderByKey(GizmoPixelShaderKey);
    InputLayout = ShaderManager->GetInputLayoutByKey(GizmoVertexShaderKey);

}
void FGizmoRenderPass::ReleaseShader()
{
    FDXDBufferManager::SafeRelease(InputLayout);
    FDXDBufferManager::SafeRelease(PixelShader);
    FDXDBufferManager::SafeRelease(VertexShader);
}


void FGizmoRenderPass::ClearRenderArr()
{
    //GizmoObjs.Empty();
}

void FGizmoRenderPass::PrepareRenderState() const
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    // 상수 버퍼 바인딩 예시
    ID3D11Buffer* PerObjectBuffer = BufferManager->GetConstantBuffer(TEXT("FPerObjectConstantBuffer"));
    ID3D11Buffer* CameraConstantBuffer = BufferManager->GetConstantBuffer(TEXT("FCameraConstantBuffer"));
    Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &PerObjectBuffer);
    Graphics->DeviceContext->VSSetConstantBuffers(1, 1, &CameraConstantBuffer);

    TArray<FString> PSBufferKeys = {
                                  TEXT("FPerObjectConstantBuffer"),
                                  TEXT("FMaterialConstants"),
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);
    //Graphics->UnbindDSV();
    //Graphics->DeviceContext->PSSetShaderResources(0, 1, &Graphics->DepthBufferSRV);
    //Graphics->DeviceContext->PSSetSamplers(0, 1, &Graphics->DepthSampler);
}

void FGizmoRenderPass::PrepareRender()
{
}

void FGizmoRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (GEngine->ActiveWorld->WorldType != EWorldType::Editor)
        return;

    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        UE_LOG(LogLevel::Error, TEXT("Gizmo RenderPass : Render : Engine is not valid."));
        return;
    }

    PrepareRenderState();
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStateDisable, 0);
    Graphics->DeviceContext->RSSetState(FEngineLoop::GraphicDevice.RasterizerStateSOLID);
    ControlMode Mode = Engine->GetEditorPlayer()->GetControlMode();
    UWorld* ActiveWorld = GEngine->ActiveWorld;
    if (!ActiveWorld)
    {
        UE_LOG(LogLevel::Error, TEXT("Gizmo RenderPass : Render : ActiveWorld is not valid."));
        return;
    }

    if (Mode == CM_TRANSLATION)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetArrowArr())
        {
            UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp);

            RenderGizmoComponent(GizmoComp, Viewport, ActiveWorld);

            Graphics->DeviceContext->RSSetState(Graphics->GetCurrentRasterizer());
            Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStateDisable, 0);
        }
    }
    else if (Mode == CM_SCALE)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetScaleArr())
        {
            UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp);


            RenderGizmoComponent(GizmoComp, Viewport, ActiveWorld);

            Graphics->DeviceContext->RSSetState(Graphics->GetCurrentRasterizer());
            Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStateDisable, 0);
        }
    }
    else if (Mode == CM_ROTATION)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetDiscArr())
        {
            UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp);

            RenderGizmoComponent(GizmoComp, Viewport, ActiveWorld);

            Graphics->DeviceContext->RSSetState(Graphics->GetCurrentRasterizer());
            Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState, 0);
        }
    }

    //ID3D11ShaderResourceView* nullSRV = nullptr;
    //Graphics->DeviceContext->PSSetShaderResources(0, 1, &nullSRV);
    //Graphics->RestoreDSV();

}

void FGizmoRenderPass::RenderGizmoComponent(UGizmoBaseComponent* GizmoComp, const std::shared_ptr<FEditorViewportClient>& Viewport, const UWorld* World)
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (Engine && !Engine->GetSelectedActor())
        return;
    // 모델 행렬.
    FMatrix Model = GizmoComp->GetWorldMatrix();

    FVector4 UUIDColor = GizmoComp->EncodeUUID() / 255.0f;

    bool Selected = (GizmoComp == Viewport->GetPickedGizmoComponent());

    FMatrix NormalMatrix = RendererHelpers::CalculateNormalMatrix(Model);

    FPerObjectConstantBuffer Data(Model, NormalMatrix, UUIDColor, Selected);

    FCameraConstantBuffer CameraData(Viewport->View, Viewport->Projection);

    BufferManager->UpdateConstantBuffer(TEXT("FPerObjectConstantBuffer"), Data);
    BufferManager->UpdateConstantBuffer(TEXT("FCameraConstantBuffer"), CameraData);

    // Gizmo가 렌더링할 StaticMesh가 없으면 렌더링하지 않음
    if (!GizmoComp->GetStaticMesh())
        return;

    OBJ::FStaticMeshRenderData* RenderData = GizmoComp->GetStaticMesh()->GetRenderData();

    if (!RenderData)
        return;

    UINT stride = sizeof(FStaticMeshVertex);

    UINT offset = 0;

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &RenderData->VertexBuffer, &stride, &offset);

    if (RenderData->IndexBuffer)
        Graphics->DeviceContext->IASetIndexBuffer(RenderData->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
    }
    else
    {
        for (int subMeshIndex = 0; subMeshIndex < RenderData->MaterialSubsets.Num(); subMeshIndex++)
        {
            int materialIndex = RenderData->MaterialSubsets[subMeshIndex].MaterialIndex;

            FSubMeshConstants SubMeshData = FSubMeshConstants(false);
            BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

            TArray<FStaticMaterial*>Materials = GizmoComp->GetStaticMesh()->GetMaterials();
            TArray<UMaterial*>OverrideMaterials = GizmoComp->GetOverrideMaterials();

            if (OverrideMaterials[materialIndex] != nullptr)
                MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[materialIndex]->GetMaterialInfo());
            else
                MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[materialIndex]->Material->GetMaterialInfo());

            uint64 startIndex = RenderData->MaterialSubsets[subMeshIndex].IndexStart;
            uint64 indexCount = RenderData->MaterialSubsets[subMeshIndex].IndexCount;

            Graphics->DeviceContext->DrawIndexed(indexCount, startIndex, 0);
        }
    }
}
