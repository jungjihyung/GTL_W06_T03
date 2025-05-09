#include "DepthBufferDebugPass.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include <wchar.h>

FDepthBufferDebugPass::FDepthBufferDebugPass()
    : Graphics(nullptr)
    , ShaderManager(nullptr)
    , SpriteVertexShader(nullptr)
    , DepthBufferPixelShader(nullptr)
    , InputLayout(nullptr)
{
}

FDepthBufferDebugPass::~FDepthBufferDebugPass()
{
    if (SpriteVertexShader) { SpriteVertexShader->Release(); SpriteVertexShader = nullptr; }
    if (DepthBufferPixelShader) { DepthBufferPixelShader->Release(); DepthBufferPixelShader = nullptr; }
    if (InputLayout) { InputLayout->Release(); InputLayout = nullptr; }
    //if (DepthStateDisable) { DepthStateDisable->Release(); DepthStateDisable = nullptr; }
    // DepthBufferSRV는 외부에서 관리하므로 해제하지 않음.
}

void FDepthBufferDebugPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    ShaderManager = InShaderManager;
    CreateSpriteResources();
    CreateShader();

    FScreenConstants sc;
    sc.ScreenSize[0] = Graphics->screenWidth;
    sc.ScreenSize[1] = Graphics->screenHeight;

    sc.Padding = { 0.0f, 0.0f };

    BufferManager->UpdateConstantBuffer(TEXT("FScreenConstants"), sc);

    Graphics->SubscribeResizeEvent([&](UINT width, UINT height)
        {
            FScreenConstants sc;
            sc.ScreenSize[0] = width;
            sc.ScreenSize[1] = height;
            sc.Padding = { 0.0f, 0.0f };

            BufferManager->UpdateConstantBuffer(TEXT("FScreenConstants"), sc);
        });
}

void FDepthBufferDebugPass::CreateSpriteResources()
{
    // 이거 왜 또 함?
    
    //D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    //dsDesc.DepthEnable = FALSE;
    //dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    //dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

    //HRESULT hr = Graphics->Device->CreateDepthStencilState(&dsDesc, &Graphics->DepthStateDisable);

}

void FDepthBufferDebugPass::CreateShader()
{
    // 입력 레이아웃 정의: POSITION과 TEXCOORD
    D3D11_INPUT_ELEMENT_DESC depthInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // 정점 셰이더 및 입력 레이아웃 생성
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout( 
        L"Shaders/DepthBufferVertexShader.hlsl",
        "mainVS",
        depthInputLayout,
        ARRAYSIZE(depthInputLayout),
        EViewModeIndex::VMI_SceneDepth, DepthBufferVertexShaderKey
    );

    // 픽셀 셰이더 생성
    hr = ShaderManager->AddPixelShader(
        L"Shaders/DepthBufferPixelShader.hlsl",
        "mainPS", EViewModeIndex::VMI_SceneDepth, DepthBufferPixelShaderKey
    );

   
    InputLayout = ShaderManager->GetInputLayoutByKey(DepthBufferVertexShaderKey);

    CreateDepthBufferSrv();
}

void FDepthBufferDebugPass::CreateDepthBufferSrv()
{
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    Graphics->Device->CreateSamplerState(&sampDesc, &Graphics->DepthSampler);

}

void FDepthBufferDebugPass::PrepareRenderState()
{

    SpriteVertexShader = ShaderManager->GetVertexShaderByKey(DepthBufferVertexShaderKey);
    DepthBufferPixelShader = ShaderManager->GetPixelShaderByKey(DepthBufferPixelShaderKey);

	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 1, nullSRV);
    
    auto DepthStencilDisableState = Graphics->DepthStateDisable;
    auto DepthBufferSRV = Graphics->DepthBufferSRV;
    auto DepthSampler = Graphics->DepthSampler;
    // 셰이더 설정
    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->FrameBufferRTV, nullptr);
    Graphics->DeviceContext->OMSetDepthStencilState(DepthStencilDisableState, 0);

    // 3. 셰이더 설정
    Graphics->DeviceContext->VSSetShader(SpriteVertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(DepthBufferPixelShader, nullptr, 0);

    // SRV & Sampler 바인딩
    Graphics->UnbindDSV();
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &DepthBufferSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &DepthSampler);
}
void FDepthBufferDebugPass::UpdateDepthBufferSRV()
{
    if (screenWidth != Graphics->screenWidth || screenHeight != Graphics->screenHeight) {

        screenWidth = Graphics->screenWidth;
        screenHeight = Graphics->screenHeight;
    }
}

void FDepthBufferDebugPass::UpdateScreenConstant(const D3D11_VIEWPORT& viewport)
{
    float sw = float(screenWidth);
    float sh = float(screenHeight);

    FScreenConstants sc;
    sc.ScreenSize[0] = sw;
    sc.ScreenSize[1] = sh;
    sc.UVOffset = { viewport.TopLeftX / sw, viewport.TopLeftY / sh };
    sc.UVScale = { viewport.Width / sw, viewport.Height / sh };
    sc.Padding = { 0.0f, 0.0f };

    BufferManager->UpdateConstantBuffer(TEXT("FScreenConstants"), sc);
    BufferManager->BindConstantBuffer(TEXT("FScreenConstants"), 0, EShaderStage::Pixel);
}

void FDepthBufferDebugPass::RenderDepthBuffer(const std::shared_ptr<FEditorViewportClient>& ActiveViewport)
{
    D3D11_VIEWPORT vp = ActiveViewport->GetD3DViewport();
    UpdateDepthBufferSRV();

    PrepareRenderState();

    UpdateScreenConstant(vp);

    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;

    BufferManager->GetQuadBuffer(VertexInfo, IndexInfo);
    
    UINT offset = 0;

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &VertexInfo.Stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    Graphics->DeviceContext->DrawIndexed(6, 0, 0);

    // 기존 DSV 복원 및 SRV 해제
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &nullSRV);

    // 기존의 DepthStencil 상태 및 렌더 타깃을 복원
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState, 0);
    Graphics->RestoreDSV();
}
