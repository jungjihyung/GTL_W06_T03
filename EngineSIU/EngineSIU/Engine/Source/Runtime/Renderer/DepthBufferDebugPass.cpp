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
    , DepthBufferSRV(nullptr)
    , DepthStateDisable(nullptr)
{
}

FDepthBufferDebugPass::~FDepthBufferDebugPass()
{
    if (SpriteVertexShader) { SpriteVertexShader->Release(); SpriteVertexShader = nullptr; }
    if (DepthBufferPixelShader) { DepthBufferPixelShader->Release(); DepthBufferPixelShader = nullptr; }
    if (InputLayout) { InputLayout->Release(); InputLayout = nullptr; }
    if (DepthStateDisable) { DepthStateDisable->Release(); DepthStateDisable = nullptr; }
    // DepthBufferSRV는 외부에서 관리하므로 해제하지 않음.
}

void FDepthBufferDebugPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    ShaderManager = InShaderManager;
    CreateSpriteResources();
    CreateShader();
}

void FDepthBufferDebugPass::CreateSpriteResources()
{
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

    HRESULT hr = Graphics->Device->CreateDepthStencilState(&dsDesc, &DepthStateDisable);

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
        nullptr, DepthBufferVertexShaderKey
    );

    // 픽셀 셰이더 생성
    hr = ShaderManager->AddPixelShader(
        L"Shaders/DepthBufferPixelShader.hlsl",
        "mainPS",nullptr, DepthBufferPixelShaderKey
    );

    // 생성된 셰이더와 입력 레이아웃 획득
    SpriteVertexShader = ShaderManager->GetVertexShaderByKey(DepthBufferVertexShaderKey);
    DepthBufferPixelShader = ShaderManager->GetPixelShaderByKey(DepthBufferPixelShaderKey);
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
    Graphics->Device->CreateSamplerState(&sampDesc, &DepthSampler);

}

void FDepthBufferDebugPass::PrepareRenderState()
{
    // 1. 이전 패스에서 바인딩된 PS 리소스 해제 (리소스 하자드 예방)
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 1, nullSRV);

    // 2. 출력 렌더 타깃을 바인딩
    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->FrameBufferRTV, nullptr);
    // depth 테스트를 비활성화한 깊이 스텐실 상태 적용
    Graphics->DeviceContext->OMSetDepthStencilState(DepthStateDisable, 0);

    // 3. 셰이더 설정
    Graphics->DeviceContext->VSSetShader(SpriteVertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(DepthBufferPixelShader, nullptr, 0);

    // 4. 디버그 패스에서 사용할 깊이 버퍼 SRV와 샘플러를 바인딩
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &DepthBufferSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &DepthSampler);
}
void FDepthBufferDebugPass::UpdateDepthBufferSRV()
{
    // 화면 크기가 변경되었으면 SRV를 재생성
    if (screenWidth != Graphics->screenWidth || screenHeight != Graphics->screenHeight) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        // 기존 SRV가 있다면 해제
        if (DepthBufferSRV) { DepthBufferSRV->Release(); DepthBufferSRV = nullptr; }

        HRESULT hr = Graphics->Device->CreateShaderResourceView(Graphics->DepthStencilBuffer, &srvDesc, &DepthBufferSRV);
        if (FAILED(hr)) {
            return;
        }
        screenWidth = Graphics->screenWidth;
        screenHeight = Graphics->screenHeight;

    }
}

void FDepthBufferDebugPass::UpdateScreenConstant(const D3D11_VIEWPORT& viewport)
{
    float sw = float(screenWidth);
    float sh = float(screenHeight);

    FScreenConstants sc;
    sc.ScreenSize = { sw, sh };
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
    
    // 렌더링 후 SRV 해제
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 1, nullSRV);

    // 기존의 DepthStencil 상태 및 렌더 타깃을 복원
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState, 0);
    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->FrameBufferRTV, Graphics->DepthStencilView);

}
