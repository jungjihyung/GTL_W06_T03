#include "FogRenderPass.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Define.h"
#include "Engine/Classes/GameFramework/Actor.h"
#include <wchar.h>
#include <UObject/UObjectIterator.h>
#include <Engine/Engine.h>
#include "PropertyEditor/ShowFlags.h"

FFogRenderPass::FFogRenderPass()
    : Graphics(nullptr)
    , ShaderManager(nullptr)
    , FogVertexShader(nullptr)
    , FogPixelShader(nullptr)
    , InputLayout(nullptr)
{
}

FFogRenderPass::~FFogRenderPass()
{
    if (FogVertexShader) { FogVertexShader->Release(); FogVertexShader = nullptr; }
    if (FogPixelShader) { FogPixelShader->Release(); FogPixelShader = nullptr; }
    if (InputLayout) { InputLayout->Release(); InputLayout = nullptr; }
    if (SceneSRV) { SceneSRV->Release(); SceneSRV = nullptr; }
    if (FogBlendState) { FogBlendState->Release(); FogBlendState = nullptr; }
    if (FogBuffer) { FogBuffer->Release(); FogBuffer = nullptr; }
    if (FogRTV) { FogRTV->Release(); FogRTV = nullptr; }
    if (FogSRV) { FogSRV->Release(); FogSRV = nullptr; }
}

void FFogRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    ShaderManager = InShaderManager;
    CreateRTV();
    CreateShader();
    CreateBlendState();
}

void FFogRenderPass::CreateShader()
{
    // 입력 레이아웃 정의: POSITION과 TEXCOORD
    D3D11_INPUT_ELEMENT_DESC fogInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // 정점 셰이더 및 입력 레이아웃 생성
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(
        L"FogVertexShader",
        L"Shaders/FogVertexShader.hlsl",
        "mainVS",
        fogInputLayout,
        ARRAYSIZE(fogInputLayout)
    );
    // 픽셀 셰이더 생성
    hr = ShaderManager->AddPixelShader(
        L"Shaders/FogPixelShader.hlsl",
        "mainPS", nullptr, FogPixelShaderKey
    );

    hr = ShaderManager->AddPixelShader(
         L"Shaders/FogQuadPixelShader.hlsl",
        "mainPS", nullptr, FogQuadPixelShaderKey
    );

    // 생성된 셰이더와 입력 레이아웃 획득
    FogVertexShader = ShaderManager->GetVertexShaderByKey(L"FogVertexShader");
    FogPixelShader = ShaderManager->GetPixelShaderByKey(FogPixelShaderKey);
    FogQuadPixelShader = ShaderManager->GetPixelShaderByKey(FogQuadPixelShaderKey);
    InputLayout = ShaderManager->GetInputLayoutByKey(L"FogVertexShader");

    CreateSceneSrv();
}

void FFogRenderPass::CreateSceneSrv()
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
    Graphics->Device->CreateSamplerState(&sampDesc, &Sampler);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    // 기존 SRV가 있다면 해제
    if (SceneSRV) { SceneSRV->Release(); SceneSRV = nullptr; }

    HRESULT hr = Graphics->Device->CreateShaderResourceView(Graphics->SceneColorBuffer, &srvDesc, &SceneSRV);
    if (FAILED(hr)) {
        return;
    }

    srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    // 기존 SRV가 있다면 해제
    if (FogSRV) { FogSRV->Release(); FogSRV = nullptr; }

    hr = Graphics->Device->CreateShaderResourceView(FogBuffer, &srvDesc, &FogSRV);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"FogSRV 생성 실패!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }
}

void FFogRenderPass::PrepareRender()
{
    for (const auto iter : TObjectRange<UHeightFogComponent>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld)
        {
            FogComponents.Add(iter);
        }
    }
}

void FFogRenderPass::ClearRenderArr()
{
    FogComponents.Empty();
}

void FFogRenderPass::PrepareRenderState(ID3D11ShaderResourceView* DepthSRV)
{
    float Color[4] = { 0,0,0,0 };
    Graphics->DeviceContext->ClearRenderTargetView(FogRTV, Color);
    ID3D11RenderTargetView* nullRTV = nullptr;
    Graphics->DeviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);
    Graphics->DeviceContext->OMSetRenderTargets(1, &FogRTV, nullptr);
    Graphics->DeviceContext->OMSetBlendState(FogBlendState, nullptr, 0xffffffff);

    // 셰이더 설정
    Graphics->DeviceContext->VSSetShader(FogVertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(FogPixelShader, nullptr, 0);

    // SRV & Sampler 바인딩
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &DepthSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Sampler);
}

void FFogRenderPass::RenderFog(const std::shared_ptr<FEditorViewportClient>& ActiveViewport, ID3D11ShaderResourceView* DepthSRV)
{
    if (ActiveViewport->GetViewMode() == EViewModeIndex::VMI_Wireframe || FogComponents.Num() <= 0
        || !(ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Fog)))
        return;

    D3D11_VIEWPORT vp = ActiveViewport->GetD3DViewport();
    CheckResize();

    UpdateScreenConstant(vp);

    PrepareRenderState(DepthSRV);

    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;

    BufferManager->GetQuadBuffer(VertexInfo, IndexInfo);

    UINT offset = 0;

    for (const auto& Fog : FogComponents)
    {
        if (Fog->GetFogDensity() > 0 && Fog->GetFogMaxOpacity() > 0)
        {
            UpdateFogConstant(ActiveViewport, Fog);

            Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &VertexInfo.Stride, &offset);
            Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
            Graphics->DeviceContext->IASetInputLayout(InputLayout);

            Graphics->DeviceContext->DrawIndexed(6, 0, 0);
        }
    }

    PrepareFinalRender();
    FinalRender();

    Graphics->DeviceContext->OMSetRenderTargets(2, Graphics->RTVs, Graphics->DepthStencilView);
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

}

void FFogRenderPass::CheckResize()
{
    // 화면 크기가 변경되었으면 SRV를 재생성
    if (screenWidth != Graphics->screenWidth || screenHeight != Graphics->screenHeight) {
        if (FogBuffer) { FogBuffer->Release(); FogBuffer = nullptr; }
        if (FogRTV) { FogRTV->Release(); FogRTV = nullptr; }
        CreateRTV();

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        // 기존 SRV가 있다면 해제
        if (SceneSRV) { SceneSRV->Release(); SceneSRV = nullptr; }

        HRESULT hr = Graphics->Device->CreateShaderResourceView(Graphics->SceneColorBuffer, &srvDesc, &SceneSRV);
        if (FAILED(hr)) {
            return;
        }

        srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        // 기존 SRV가 있다면 해제
        if (FogSRV) { FogSRV->Release(); FogSRV = nullptr; }

        hr = Graphics->Device->CreateShaderResourceView(FogBuffer, &srvDesc, &FogSRV);
        if (FAILED(hr))
        {
            MessageBox(NULL, L"FogSRV 생성 실패!", L"Error", MB_ICONERROR | MB_OK);
            return;
        }

        screenWidth = Graphics->screenWidth;
        screenHeight = Graphics->screenHeight;
    }
}

void FFogRenderPass::UpdateScreenConstant(const D3D11_VIEWPORT& viewport)
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

void FFogRenderPass::UpdateFogConstant(const std::shared_ptr<FEditorViewportClient>& ActiveViewport, UHeightFogComponent* Fog)
{
    FMatrix View = ActiveViewport->View;
    FMatrix Projection = ActiveViewport->GetProjectionMatrix();
    FMatrix ViewProj = View * Projection;
    FMatrix Inverse = FMatrix::Inverse(ViewProj);
    FFogConstants Constants; 
    {
        Constants.InvViewProj = Inverse;
        Constants.FogColor = Fog->GetFogColor();
        Constants.CameraPos = ActiveViewport->ViewTransformPerspective.GetLocation();
        Constants.FogDensity = Fog->GetFogDensity();
        Constants.FogHeightFalloff = Fog->GetFogHeightFalloff();
        Constants.StartDistance = Fog->GetStartDistance();
        Constants.FogCutoffDistance = Fog->GetFogCutoffDistance();
        Constants.FogMaxOpacity = Fog->GetFogMaxOpacity();
        Constants.FogPosition = Fog->GetWorldLocation();
        Constants.CameraNear = ActiveViewport->nearPlane;
        Constants.CameraFar = ActiveViewport->farPlane;
    }
    //상수버퍼 업데이트
    BufferManager->UpdateConstantBuffer(TEXT("FFogConstants"), Constants);
    //상수버퍼 바인딩
    ID3D11Buffer* FogConstantBuffer = BufferManager->GetConstantBuffer(TEXT("FFogConstants"));
    BufferManager->BindConstantBuffer(TEXT("FFogConstants"), 1, EShaderStage::Pixel);
}

void FFogRenderPass::CreateBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = Graphics->Device->CreateBlendState(&blendDesc, &FogBlendState);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"AlphaBlendState 생성에 실패했습니다!", L"Error", MB_ICONERROR | MB_OK);
    }
}

void FFogRenderPass::PrepareFinalRender()
{
    // 셰이더 설정
    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->FrameBufferRTV, nullptr);
    Graphics->DeviceContext->OMSetBlendState(FogBlendState, nullptr, 0xffffffff);

    Graphics->DeviceContext->VSSetShader(FogVertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(FogQuadPixelShader, nullptr, 0);

    // SRV & Sampler 바인딩
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &SceneSRV);
    Graphics->DeviceContext->PSSetShaderResources(1, 1, &FogSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Sampler);
}

void FFogRenderPass::FinalRender()
{
    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;

    BufferManager->GetQuadBuffer(VertexInfo, IndexInfo);

    UINT offset = 0;

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &VertexInfo.Stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    Graphics->DeviceContext->DrawIndexed(6, 0, 0);
}

void FFogRenderPass::CreateRTV()
{
    Graphics->CreateRTV(FogBuffer, FogRTV);
}
