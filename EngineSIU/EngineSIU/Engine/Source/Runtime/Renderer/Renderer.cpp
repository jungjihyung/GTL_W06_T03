
#include "Renderer.h"
#include "World/World.h"
#include "Engine/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "RendererHelpers.h"
#include "StaticMeshRenderPass.h"
#include "BillboardRenderPass.h"
#include "GizmoRenderPass.h"
#include "UpdateLightBufferPass.h"
#include "LineRenderPass.h"
#include "DepthBufferDebugPass.h"
#include "FogRenderPass.h"
#include "LightCullPass.h"
#include <UObject/UObjectIterator.h>
#include <UObject/Casts.h>
#include "GameFrameWork/Actor.h"


//------------------------------------------------------------------------------
// 초기화 및 해제 관련 함수
//------------------------------------------------------------------------------
void FRenderer::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;

    ShaderManager = new FDXDShaderManager(Graphics->Device, Graphics);
    ShaderHotReload = new FShaderHotReload(ShaderManager);

    StaticMeshRenderPass = new FStaticMeshRenderPass();
    BillboardRenderPass = new FBillboardRenderPass();
    GizmoRenderPass = new FGizmoRenderPass();
    UpdateLightBufferPass = new FUpdateLightBufferPass();
    LineRenderPass = new FLineRenderPass();
    DepthBufferDebugPass = new FDepthBufferDebugPass();
    FogRenderPass = new FFogRenderPass();
    LightCullPass = new FLightCullPass();

    StaticMeshRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    BillboardRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    GizmoRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    UpdateLightBufferPass->Initialize(BufferManager, Graphics, ShaderManager);
    LineRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    DepthBufferDebugPass->Initialize(BufferManager, Graphics, ShaderManager);
    FogRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    LightCullPass->Initialize(BufferManager, Graphics, ShaderManager);
    
    ShaderHotReload->CreateShaderHotReloadThread();

    CreateConstantBuffers();
}

void FRenderer::Release()
{
}


void FRenderer::ChangeViewMode(EViewModeIndex evi)
{
    if (evi == EViewModeIndex::VMI_SceneDepth)
        IsSceneDepth = true;
    else
        IsSceneDepth = false;
}

//------------------------------------------------------------------------------
// 사용하는 모든 상수 버퍼 생성
//------------------------------------------------------------------------------
void FRenderer::CreateConstantBuffers()
{
    UINT perObjectBufferSize = sizeof(FPerObjectConstantBuffer);
    BufferManager->CreateBufferGeneric<FPerObjectConstantBuffer>("FPerObjectConstantBuffer", nullptr, perObjectBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT cameraConstantBufferSize = sizeof(FCameraConstantBuffer);
    BufferManager->CreateBufferGeneric<FCameraConstantBuffer>("FCameraConstantBuffer", nullptr, cameraConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT subUVBufferSize = sizeof(FSubUVConstant);
    BufferManager->CreateBufferGeneric<FSubUVConstant>("FSubUVConstant", nullptr, subUVBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT materialBufferSize = sizeof(FMaterialConstants);
    BufferManager->CreateBufferGeneric<FMaterialConstants>("FMaterialConstants", nullptr, materialBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT subMeshBufferSize = sizeof(FSubMeshConstants);
    BufferManager->CreateBufferGeneric<FSubMeshConstants>("FSubMeshConstants", nullptr, subMeshBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT textureBufferSize = sizeof(FTextureConstants);
    BufferManager->CreateBufferGeneric<FTextureConstants>("FTextureConstants", nullptr, textureBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT lightingBufferSize = sizeof(FLightBuffer);
    BufferManager->CreateBufferGeneric<FLightBuffer>("FLightBuffer", nullptr, lightingBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ScreenConstantsBufferSize = sizeof(FScreenConstants);
    BufferManager->CreateBufferGeneric<FScreenConstants>("FScreenConstants", nullptr, ScreenConstantsBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT FogConstantBufferSize = sizeof(FFogConstants);
    BufferManager->CreateBufferGeneric<FFogConstants>("FFogConstants", nullptr, FogConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT FPrimitiveCountsSize = sizeof(FPrimitiveCounts);
    BufferManager->CreateBufferGeneric<FPrimitiveCounts>("FPrimitiveCounts", nullptr, FPrimitiveCountsSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT FGridParametersSize = sizeof(FGridParameters);
    BufferManager->CreateBufferGeneric<FGridParameters>("FGridParameters", nullptr, FGridParametersSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FRenderer::ReleaseConstantBuffer()
{
    BufferManager->ReleaseConstantBuffer();
}

void FRenderer::PrepareRender()
{
    StaticMeshRenderPass->PrepareRenderState();
    StaticMeshRenderPass->PrepareRender();
    GizmoRenderPass->PrepareRender();
    BillboardRenderPass->PrepareRender();
    UpdateLightBufferPass->PrepareRender();
    FogRenderPass->PrepareRender();
}

void FRenderer::ClearRenderArr()
{
    StaticMeshRenderPass->ClearRenderArr();
    BillboardRenderPass->ClearRenderArr();
    GizmoRenderPass->ClearRenderArr();
    UpdateLightBufferPass->ClearRenderArr();
    FogRenderPass->ClearRenderArr();
}

void FRenderer::Render(const std::shared_ptr<FEditorViewportClient>& ActiveViewport)
{
    Graphics->DeviceContext->RSSetViewports(1, &ActiveViewport->GetD3DViewport());


    Graphics->ChangeRasterizer(ActiveViewport->GetViewMode());

    ChangeViewMode(ActiveViewport->GetViewMode());

    UpdateLightBufferPass->Render(ActiveViewport);
    LightCullPass->Render(ActiveViewport);

    StaticMeshRenderPass->SwitchShaderLightingMode(ActiveViewport->GetViewMode());
    StaticMeshRenderPass->Render(ActiveViewport);
    BillboardRenderPass->Render(ActiveViewport);


    if (IsSceneDepth)
    {
        DepthBufferDebugPass->RenderDepthBuffer(ActiveViewport);
    }

    if (!IsSceneDepth)
    {
        DepthBufferDebugPass->UpdateDepthBufferSRV();

        FogRenderPass->RenderFog(ActiveViewport, Graphics->DepthBufferSRV);
    }

    // Light뷰모드일 경우
    if (ActiveViewport->GetViewMode() == static_cast<uint64>(EViewModeIndex::VMI_Light))
    {
        ClearRenderArr();

        return;
    }

    LineRenderPass->Render(ActiveViewport);
    GizmoRenderPass->Render(ActiveViewport);
    ClearRenderArr();
}
