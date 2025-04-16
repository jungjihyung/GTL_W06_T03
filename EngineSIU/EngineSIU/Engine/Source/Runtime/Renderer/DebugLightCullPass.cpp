#include "DebugLightCullPass.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "EngineLoop.h"
#include "Editor/UnrealEd/EditorViewportClient.h"
#include "PropertyEditor/ShowFlags.h"

void FDebugLightCullPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManage;

    CreateShader();
}

void FDebugLightCullPass::PrepareRender()
{
}

void FDebugLightCullPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (Viewport->GetViewMode() != EViewModeIndex::VMI_LightDebug)
        return;

    // 여기에 한 프레임이라도 그리면 뎁스 의미가 없어진다
    Graphics->UnbindDSV();

    BufferManager->BindConstantBuffer(TEXT("FScreenConstants"), 2, EShaderStage::Pixel);

    Graphics->DeviceContext->PSSetShader(DebugPixelShader, nullptr, 0);
    Graphics->DeviceContext->VSSetShader(DebugVertexShader, nullptr, 0);

    Graphics->DeviceContext->PSSetShaderResources(3, 1, &Graphics->LightIndexCountSRV);


    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;

    BufferManager->GetQuadBuffer(VertexInfo, IndexInfo);

    UINT offset = 0;

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &VertexInfo.Stride, &offset);

    Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);


    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->DrawIndexed(6, 0, 0);
    // 기존 DSV 복원 및 SRV 해제
    Graphics->RestoreDSV();
}

void FDebugLightCullPass::ClearRenderArr()
{
}

void FDebugLightCullPass::CreateShader()
{
    size_t Key;
    HRESULT hr = ShaderManager->AddPixelShader(L"Shaders/LightCullDebugShader.hlsl", "mainPS", EViewModeIndex::VMI_LightDebug, Key);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create LightCullDebugShader!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }
    DebugPixelShader = ShaderManager->GetPixelShaderByKey(Key);


    size_t LightDebugVertexShaderKey;
    // 입력 레이아웃 정의: POSITION과 TEXCOORD
    D3D11_INPUT_ELEMENT_DESC LightDebugInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // 정점 셰이더 및 입력 레이아웃 생성
    hr = ShaderManager->AddVertexShaderAndInputLayout(
        L"Shaders/LightCullDebugShader.hlsl",
        "mainVS",
        LightDebugInputLayout,
        ARRAYSIZE(LightDebugInputLayout),
        EViewModeIndex::VMI_LightDebug, LightDebugVertexShaderKey
    );

    DebugVertexShader = ShaderManager->GetVertexShaderByKey(LightDebugVertexShaderKey);
    InputLayout = ShaderManager->GetInputLayoutByKey(LightDebugVertexShaderKey);
}
