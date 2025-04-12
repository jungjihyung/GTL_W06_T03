#include "StaticMeshRenderPass.h"

#include "EngineLoop.h"
#include "World/World.h"

#include "RendererHelpers.h"
#include "Math/JungleMath.h"

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "Components/StaticMeshComponent.h"

#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"

#include "PropertyEditor/ShowFlags.h"

#include "UnrealEd/EditorViewportClient.h"


FStaticMeshRenderPass::FStaticMeshRenderPass()
    : VertexShader(nullptr)
    , PixelShader(nullptr)
    , InputLayout(nullptr)
    , Stride(0)
    , BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FStaticMeshRenderPass::~FStaticMeshRenderPass()
{
    ReleaseShader();
    if (ShaderManager)
    {
        delete ShaderManager;
        ShaderManager = nullptr;
    }
}

void FStaticMeshRenderPass::CreateShader()
{
    D3D11_INPUT_ELEMENT_DESC StaticMeshLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"MATERIAL_INDEX", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3D11_INPUT_ELEMENT_DESC TextureLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    Stride = sizeof(FStaticMeshVertex);


    // Gouraud 조명 모델 (Lit 모드)
    D3D_SHADER_MACRO DefineLit_Gouraud[] =
    {
        { "LIT_MODE", "1" },
        { "LIGHTING_MODEL_GOURAUD", "1" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { nullptr, nullptr }
    };

    // Lambert 조명 모델 (Lit 모드)
    D3D_SHADER_MACRO DefineLit_Lambert[] =
    {
        { "LIT_MODE", "1" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "1" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { nullptr, nullptr }
    };

    // Phong 조명 모델 (Lit 모드)
    D3D_SHADER_MACRO DefineLit_Phong[] =
    {
        { "LIT_MODE", "1" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "1" },
        { nullptr, nullptr }
    };

    D3D_SHADER_MACRO DefineUnLit[] =
    {
        { "LIT_MODE", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { nullptr, nullptr }
    };

    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"Shaders/StaticMeshVertexShader.hlsl", "mainVS",
        StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc), DefineLit_Gouraud, GouraudVertexShaderKey);

    hr = ShaderManager->AddVertexShaderAndInputLayout(L"Shaders/StaticMeshVertexShader.hlsl", "mainVS",
        StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc), DefineLit_Lambert, LambertVertexShaderKey);

    hr = ShaderManager->AddVertexShaderAndInputLayout(L"Shaders/StaticMeshVertexShader.hlsl", "mainVS",
        StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc), DefineLit_Phong, PhongVertexShaderKey);

    hr = ShaderManager->AddVertexShaderAndInputLayout(L"Shaders/StaticMeshVertexShader.hlsl", "mainVS",
        StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc), DefineUnLit, UnlitVertexShaderKey);


    hr = ShaderManager->AddPixelShader(L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefineLit_Gouraud, GouraudPixelShaderKey);
    hr = ShaderManager->AddPixelShader(L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefineLit_Lambert, LambertPixelShaderKey);
    hr = ShaderManager->AddPixelShader(L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefineLit_Phong, PhongPixelShaderKey);
    hr = ShaderManager->AddPixelShader(L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefineUnLit, UnlitVertexShaderKey);

    VertexShader = ShaderManager->GetVertexShaderByKey(PhongVertexShaderKey);

    PixelShader = ShaderManager->GetPixelShaderByKey(PhongPixelShaderKey);

    InputLayout = ShaderManager->GetInputLayoutByKey(PhongVertexShaderKey);

}
void FStaticMeshRenderPass::ReleaseShader()
{
    FDXDBufferManager::SafeRelease(InputLayout);
    FDXDBufferManager::SafeRelease(PixelShader);
    FDXDBufferManager::SafeRelease(VertexShader);
}

void FStaticMeshRenderPass::SwitchShaderLightingMode(EViewModeIndex evi)
{
    switch (evi)
    {
    case EViewModeIndex::VMI_Lit_Gouraud:
        VertexShader = ShaderManager->GetVertexShaderByKey(GouraudVertexShaderKey);
        PixelShader = ShaderManager->GetPixelShaderByKey(GouraudPixelShaderKey);  
        break;
    case EViewModeIndex::VMI_Lit_Lambert:
        VertexShader = ShaderManager->GetVertexShaderByKey(LambertVertexShaderKey);
        PixelShader = ShaderManager->GetPixelShaderByKey(LambertPixelShaderKey);
        break;   
    case EViewModeIndex::VMI_Lit_Phong:
        VertexShader = ShaderManager->GetVertexShaderByKey(PhongVertexShaderKey);
        PixelShader = ShaderManager->GetPixelShaderByKey(PhongPixelShaderKey);
        break;
    case VMI_Unlit:
    case VMI_Wireframe:
    case VMI_SceneDepth:

        VertexShader = ShaderManager->GetVertexShaderByKey(UnlitVertexShaderKey);
        PixelShader = ShaderManager->GetPixelShaderByKey(UnlitPixelShaderKey);
        break;
    }
}


void FStaticMeshRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateShader();
}

void FStaticMeshRenderPass::PrepareRender()
{
    for (const auto iter : TObjectRange<UStaticMeshComponent>())
    {
        if (!Cast<UGizmoBaseComponent>(iter) && iter->GetWorld() == GEngine->ActiveWorld)
        {
            StaticMeshObjs.Add(iter);
        }
    }
}

void FStaticMeshRenderPass::PrepareRenderState() const
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    TArray<FString> VSBufferKeys = {
                              TEXT("FPerObjectConstantBuffer"),
                              TEXT("FCameraConstantBuffer"),
                              TEXT("FLightBuffer"),
                              TEXT("FMaterialConstants"),
    };


    BufferManager->BindConstantBuffers(VSBufferKeys, 0, EShaderStage::Vertex);

    TArray<FString> PSBufferKeys = {
                                  TEXT("FCameraConstantBuffer"),
                                  TEXT("FLightBuffer"),
                                  TEXT("FMaterialConstants"),
                                  TEXT("FSubMeshConstants"),
                                  TEXT("FTextureConstants")
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 1, EShaderStage::Pixel);
}

void FStaticMeshRenderPass::UpdatePerObjectConstant(const FMatrix& Model, const FMatrix& View, const FMatrix& Projection, const FVector4& UUIDColor, bool Selected) const
{
    FMatrix NormalMatrix = RendererHelpers::CalculateNormalMatrix(Model);
    FPerObjectConstantBuffer Data(Model, NormalMatrix, UUIDColor, Selected);
    BufferManager->UpdateConstantBuffer(TEXT("FPerObjectConstantBuffer"), Data);

}


void FStaticMeshRenderPass::RenderPrimitive(OBJ::FStaticMeshRenderData* RenderData, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int SelectedSubMeshIndex) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &RenderData->VertexBuffer, &Stride, &offset);
    if (RenderData->IndexBuffer)
        Graphics->DeviceContext->IASetIndexBuffer(RenderData->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    if (RenderData->MaterialSubsets.Num() == 0) {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
        return;
    }

    for (int subMeshIndex = 0; subMeshIndex < RenderData->MaterialSubsets.Num(); subMeshIndex++) {

        int materialIndex = RenderData->MaterialSubsets[subMeshIndex].MaterialIndex;

        FSubMeshConstants SubMeshData = (subMeshIndex == SelectedSubMeshIndex) ? FSubMeshConstants(true) : FSubMeshConstants(false);

        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

        if (OverrideMaterials[materialIndex] != nullptr)
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[materialIndex]->GetMaterialInfo());
        else
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[materialIndex]->Material->GetMaterialInfo());

        uint64 startIndex = RenderData->MaterialSubsets[subMeshIndex].IndexStart;
        uint64 indexCount = RenderData->MaterialSubsets[subMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(indexCount, startIndex, 0);
    }
}


void FStaticMeshRenderPass::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
    Graphics->DeviceContext->Draw(numVertices, 0);
}

void FStaticMeshRenderPass::RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

void FStaticMeshRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (!(Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Primitives)))
        return;

    PrepareRenderState();

    for (UStaticMeshComponent* Comp : StaticMeshObjs)
    {
        if (!Comp || !Comp->GetStaticMesh())
            continue;

        FMatrix Model = Comp->GetWorldMatrix();

        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;

        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        bool Selected = (Engine && Engine->GetSelectedActor() == Comp->GetOwner());

        UpdatePerObjectConstant(Model, Viewport->GetViewMatrix(), Viewport->GetProjectionMatrix(), UUIDColor, Selected);
        FCameraConstantBuffer CameraData(Viewport->GetViewMatrix(), Viewport->GetProjectionMatrix(), Viewport->ViewTransformPerspective.GetLocation(), 0);
        BufferManager->UpdateConstantBuffer(TEXT("FCameraConstantBuffer"), CameraData);

        OBJ::FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();

        if (RenderData == nullptr)
            continue;

        RenderPrimitive(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetWorldLocation(), Model);
        }
    }
}

void FStaticMeshRenderPass::ClearRenderArr()
{
    StaticMeshObjs.Empty();
}

