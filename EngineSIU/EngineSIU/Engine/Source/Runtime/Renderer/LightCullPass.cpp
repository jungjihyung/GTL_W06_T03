#include "LightCullPass.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "EngineLoop.h"
#include "Editor/UnrealEd/EditorViewportClient.h"
#include "PropertyEditor/ShowFlags.h"




void FLightCullPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManage;

    CreateVisibleLightBuffer();
    CreateVisibleLightUAV();
    CreateVisibleLightSRV();
    CreateLightIndexCountBuffer();
    CreateLightIndexCountUAV();
    CreateLightIndexCountSRV();

    Graphics->SubscribeResizeEvent([&](UINT width, UINT height) {

        if (Graphics->VisibleLightBuffer)
        {
            Graphics->VisibleLightBuffer->Release();
            Graphics->VisibleLightBuffer = nullptr;
        }
        if (Graphics->VisibleLightUAV)
        {
            Graphics->VisibleLightUAV->Release();
            Graphics->VisibleLightUAV = nullptr;
        }
        if (Graphics->VisibleLightSRV)
        {
            Graphics->VisibleLightSRV->Release();
            Graphics->VisibleLightSRV = nullptr;
        }
        if (Graphics->LightIndexCountBuffer)
        {
            Graphics->LightIndexCountBuffer->Release();
            Graphics->LightIndexCountBuffer = nullptr;
        }
        if (Graphics->LightIndexCountUAV)
        {
            Graphics->LightIndexCountUAV->Release();
            Graphics->LightIndexCountUAV = nullptr;
        }
        if (Graphics->LightIndexCountSRV)
        {
            Graphics->LightIndexCountSRV->Release();
            Graphics->LightIndexCountSRV = nullptr;
        }

        CreateVisibleLightBuffer();
        CreateVisibleLightUAV();
        CreateVisibleLightSRV();
        CreateLightIndexCountBuffer();
        CreateLightIndexCountUAV();
        CreateLightIndexCountSRV();
        });

	CreateShader();
}

void FLightCullPass::PrepareRender()
{

}

void FLightCullPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // 함수 이름은 Render지만..
    ID3D11ShaderResourceView* NullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 4, NullSRVs);  // 모든 슬롯 초기화
    Graphics->DeviceContext->VSSetShaderResources(0, 4, NullSRVs);  // 모든 슬롯 초기화

    ID3D11ComputeShader* computeShader = ShaderManager->GetComputeShaderByKey(ShaderKey);
    if (!computeShader)
    {
        MessageBox(nullptr, L"LightCullComputeShader is not valid!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }
    // 0. 스크린 상수버퍼 업데이트
    FScreenConstants sc;
    sc.ScreenSize[0] = Graphics->screenWidth;
    sc.ScreenSize[1] = Graphics->screenHeight;
    sc.Padding = { 0.0f, 0.0f };
    sc.UVOffset = { Viewport->GetD3DViewport().TopLeftX / Graphics->screenWidth, Viewport->GetD3DViewport().TopLeftY / Graphics->screenHeight };
    sc.UVScale = { Viewport->GetD3DViewport().Width / Graphics->screenWidth, Viewport->GetD3DViewport().Height / Graphics->screenWidth };
    BufferManager->UpdateConstantBuffer(TEXT("FScreenConstants"), sc);

    // 1. 카메라 상수버퍼 업데이트
    FCameraConstantBuffer cameraData;
    cameraData.View = Viewport->GetViewMatrix();
    cameraData.Projection = Viewport->GetProjectionMatrix();
    cameraData.InvProjection = FMatrix::Inverse(Viewport->GetProjectionMatrix());
    cameraData.CameraNear = Viewport->nearPlane;
    cameraData.CameraFar = Viewport->farPlane;
    BufferManager->UpdateConstantBuffer(TEXT("FCameraConstantBuffer"), cameraData);

    // 1. 컴퓨트 셰이더 바인드
    Graphics->DeviceContext->CSSetShader(computeShader, nullptr, 0);

    // 2. 버퍼 초기화
    // 2.1 LightIndexCountBuffer 초기화
    UINT clearValues[4] = { 0, 0, 0, 0 };
    Graphics->DeviceContext->ClearUnorderedAccessViewUint(Graphics->VisibleLightUAV, clearValues);
    Graphics->DeviceContext->ClearUnorderedAccessViewUint(Graphics->LightIndexCountUAV, clearValues);

    // 2. SRV 및 UAV 바인드 -> Depth는 SRV만 주고, Light정보는 이미 cb로 넘기고 있다.
    Graphics->UnbindDSV();
    if(Graphics->DepthBufferSRV)
        Graphics->DeviceContext->CSSetShaderResources(0, 1, &Graphics->DepthBufferSRV); 

    if(Graphics->LightBufferSRV)
        Graphics->DeviceContext->CSSetShaderResources(1, 1, &Graphics->LightBufferSRV); // LightBufferSRV 바인드

    ID3D11UnorderedAccessView* uavs[] = { Graphics->VisibleLightUAV, Graphics->LightIndexCountUAV};
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    // 3. 상수버퍼 바인드
    TArray<FString> PSBufferKeys = {
                                  TEXT("FPerObjectConstantBuffer"),
                                  TEXT("FCameraConstantBuffer"),
                                  TEXT("FScreenConstants"),
                                  TEXT("FLightBuffer")
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Compute);

    // 4. 디스패치 호출
    UINT tileCountX = (Graphics->screenWidth + TILE_SIZE - 1) / TILE_SIZE;
    UINT tileCountY = (Graphics->screenHeight + TILE_SIZE - 1) / TILE_SIZE;
    Graphics->DeviceContext->Dispatch(tileCountX, tileCountY, 1);

    // 5. UAV 언바인드
    ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr, nullptr };
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

	// 6. 뎁스 srv 언바인드
    ID3D11ShaderResourceView* nullSRVs = nullptr;
	Graphics->DeviceContext->CSSetShaderResources(0, 1, &nullSRVs);
    Graphics->RestoreDSV();

    //여기서 뎁스버퍼 클리어한다
    Graphics->DeviceContext->ClearDepthStencilView(Graphics->DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // 깊이 버퍼 초기화 추가
}

void FLightCullPass::ClearRenderArr()
{
}

void FLightCullPass::CreateShader()
{
	HRESULT hr = ShaderManager->AddComputeShader(L"Shaders/LightCullingComputeShader.hlsl", "mainCS", ShaderKey);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create LightCullComputeShader!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }
}


void FLightCullPass::CreateVisibleLightBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(UINT) * GetMaxTileCount() * MAX_LIGHTS_PER_TILE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU에서 쓰기 가능
    desc.StructureByteStride = sizeof(UINT);
    HRESULT hr = Graphics->Device->CreateBuffer(&desc, nullptr, &Graphics->VisibleLightBuffer);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create VisibleLightIndicesBuffer!", L"Error", MB_ICONERROR | MB_OK);
        return;
    };
}

void FLightCullPass::CreateVisibleLightUAV()
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0; // 명시적으로 시작 인덱스 지정
    desc.Buffer.Flags = 0; // D3D11_BUFFER_UAV_FLAG_COUNTER 등 필요 시 설정
    desc.Buffer.NumElements = GetMaxTileCount() * MAX_LIGHTS_PER_TILE;
    HRESULT hr = Graphics->Device->CreateUnorderedAccessView(Graphics->VisibleLightBuffer, &desc, &Graphics->VisibleLightUAV);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create VisibleLightIndicesUAV!", L"Error", MB_ICONERROR | MB_OK);
        return;
    };
}

void FLightCullPass::CreateVisibleLightSRV()
{
    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0; // 명시적으로 시작 인덱스 지정
    desc.Buffer.ElementOffset = 0; // 오프셋 필요 시 설정
    desc.Buffer.NumElements = GetMaxTileCount() * MAX_LIGHTS_PER_TILE;
    HRESULT hr = Graphics->Device->CreateShaderResourceView(Graphics->VisibleLightBuffer, &desc, &Graphics->VisibleLightSRV);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create VisibleLightIndicesSRV!", L"Error", MB_ICONERROR | MB_OK);
        return;
    };
}

void FLightCullPass::CreateLightIndexCountBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(UINT) * GetMaxTileCount();
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU에서 쓰기 가능

    HRESULT hr = Graphics->Device->CreateBuffer(&desc, nullptr, &Graphics->LightIndexCountBuffer);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create LightIndexCountBuffer!", L"Error", MB_ICONERROR | MB_OK);
        return;
    };
}

void FLightCullPass::CreateLightIndexCountUAV()
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_R32_UINT; // RW버퍼는 포맷 필요
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0; // 명시적으로 시작 인덱스 지정
    desc.Buffer.Flags = 0; // D3D11_BUFFER_UAV_FLAG_COUNTER 등 필요 시 설정
    desc.Buffer.NumElements = GetMaxTileCount();
    HRESULT hr = Graphics->Device->CreateUnorderedAccessView(Graphics->LightIndexCountBuffer, &desc, &Graphics->LightIndexCountUAV);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create LightIndexCountUAV!", L"Error", MB_ICONERROR | MB_OK);
        return;
    };
}

void FLightCullPass::CreateLightIndexCountSRV()
{
    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_R32_UINT;
    desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    desc.Buffer.NumElements = GetMaxTileCount();
    desc.Buffer.FirstElement = 0; // 명시적으로 시작 인덱스 지정
    desc.Buffer.ElementOffset = 0; // 오프셋 필요 시 설정
    HRESULT hr = Graphics->Device->CreateShaderResourceView(Graphics->LightIndexCountBuffer, &desc, &Graphics->LightIndexCountSRV);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create LightIndexCountSRV!", L"Error", MB_ICONERROR | MB_OK);
        return;
    };
}

UINT FLightCullPass::GetMaxTileCount() const
{
    return ceil(Graphics->screenWidth / TILE_SIZE) * ceil(Graphics->screenHeight / TILE_SIZE);
}

