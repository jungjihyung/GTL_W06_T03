#include "GraphicDevice.h"
#include <cwchar>
#include <Components/HeightFogComponent.h>
#include <UObject/UObjectIterator.h>
#include <Engine/Engine.h>
#include "PropertyEditor/ShowFlags.h"

void FGraphicsDevice::Initialize(HWND hWindow)
{
    CreateDeviceAndSwapChain(hWindow);
    CreateFrameBuffer();
    CreateDepthStencilBuffer(hWindow);
    CreateDepthStencilState();
    CreateRasterizerState();
    CreateAlphaBlendState();
    CurrentRasterizer = RasterizerStateSOLID;
}

void FGraphicsDevice::CreateDeviceAndSwapChain(HWND hWindow)
{
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL featurelevels[] = {D3D_FEATURE_LEVEL_11_0};

    // 스왑 체인 설정 구조체 초기화
    SwapchainDesc.BufferDesc.Width = 0;                           // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Height = 0;                          // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
    SwapchainDesc.SampleDesc.Count = 1;                           // 멀티 샘플링 비활성화
    SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // 렌더 타겟으로 사용
    SwapchainDesc.BufferCount = 2;                                // 더블 버퍼링
    SwapchainDesc.OutputWindow = hWindow;                         // 렌더링할 창 핸들
    SwapchainDesc.Windowed = TRUE;                                // 창 모드
    SwapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;     // 스왑 방식

    // 디바이스와 스왑 체인 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
        &SwapchainDesc, &SwapChain, &Device, nullptr, &DeviceContext
    );

    if (FAILED(hr))
    {
        MessageBox(hWindow, L"CreateDeviceAndSwapChain failed!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // 스왑 체인 정보 가져오기 (이후에 사용을 위해)
    SwapChain->GetDesc(&SwapchainDesc);
    screenWidth = SwapchainDesc.BufferDesc.Width;
    screenHeight = SwapchainDesc.BufferDesc.Height;
}


void FGraphicsDevice::CreateDepthStencilBuffer(HWND hWindow)
{
    RECT clientRect;
    GetClientRect(hWindow, &clientRect);
    UINT width = clientRect.right - clientRect.left;
    UINT height = clientRect.bottom - clientRect.top;

    // 깊이/스텐실 텍스처 생성
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;                          // 텍스처 너비 설정
    descDepth.Height = height;                        // 텍스처 높이 설정
    descDepth.MipLevels = 1;                          // 미맵 레벨 수 (1로 설정하여 미맵 없음)
    descDepth.ArraySize = 1;                          // 텍스처 배열의 크기 (1로 단일 텍스처)
    descDepth.Format = DXGI_FORMAT_R32_TYPELESS; // 24비트 깊이와 8비트 스텐실을 위한 포맷
    descDepth.SampleDesc.Count = 1;                   // 멀티샘플링 설정 (1로 단일 샘플)
    descDepth.SampleDesc.Quality = 0;                 // 샘플 퀄리티 설정
    descDepth.Usage = D3D11_USAGE_DEFAULT;            // 텍스처 사용 방식
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;   // 깊이 스텐실 뷰로 바인딩 설정
    descDepth.CPUAccessFlags = 0;                     // CPU 접근 방식 설정
    descDepth.MiscFlags = 0;                          // 기타 플래그 설정

    HRESULT hr = Device->CreateTexture2D(&descDepth, nullptr, &DepthStencilBuffer);

    if (FAILED(hr))
    {
        MessageBox(hWindow, L"Failed to create depth stencilBuffer!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }


    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;        // 깊이 스텐실 포맷
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // 뷰 타입 설정 (2D 텍스처)
    descDSV.Texture2D.MipSlice = 0;                        // 사용할 미맵 슬라이스 설정

    hr = Device->CreateDepthStencilView(
        DepthStencilBuffer, // Depth stencil texture
        &descDSV,           // Depth stencil desc
        &DepthStencilView
    ); // [out] Depth stencil view

    if (FAILED(hr))
    {
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to create depth stencil view! HRESULT: 0x%08X", hr);
        MessageBox(hWindow, errorMsg, L"Error", MB_ICONERROR | MB_OK);
        return;
    }
}

void FGraphicsDevice::CreateDepthStencilState()
{
    // DepthStencil 상태 설명 설정
    D3D11_DEPTH_STENCIL_DESC dsDesc;

    // Depth test parameters
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    dsDesc.StencilEnable = true;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    //// DepthStencil 상태 생성
    HRESULT hr = Device->CreateDepthStencilState(&dsDesc, &DepthStencilState);
    if (FAILED(hr))
    {
        // 오류 처리
        return;
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;                         // 깊이 테스트 유지
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 버퍼에 쓰지 않음
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;         // 깊이 비교를 항상 통과
    Device->CreateDepthStencilState(&depthStencilDesc, &DepthStateDisable);
}

void FGraphicsDevice::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID;
    rasterizerdesc.CullMode = D3D11_CULL_BACK;
    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerStateSOLID);

    rasterizerdesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerdesc.CullMode = D3D11_CULL_BACK;
    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerStateWIREFRAME);
}


void FGraphicsDevice::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // 남아있는 GPU 명령 실행
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
}

void FGraphicsDevice::CreateFrameBuffer()
{
    // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
    SwapChain->GetBuffer(0, IID_PPV_ARGS(&FrameBuffer));

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;      // 색상 포맷
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = screenWidth;
    textureDesc.Height = screenHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    Device->CreateTexture2D(&textureDesc, nullptr, &UUIDFrameBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC UUIDFrameBufferRTVDesc = {};
    UUIDFrameBufferRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;           // 색상 포맷
    UUIDFrameBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(UUIDFrameBuffer, &UUIDFrameBufferRTVDesc, &UUIDFrameBufferRTV);

    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = screenWidth;
    TextureDesc.Height = screenHeight;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;
    Device->CreateTexture2D(&TextureDesc, nullptr, &SceneColorBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC SceneColorRTVDesc = {};
    SceneColorRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 색상 포맷
    SceneColorRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(SceneColorBuffer, &SceneColorRTVDesc, &SceneColorRTV);

    RTVs[0] = FrameBufferRTV;
    RTVs[1] = UUIDFrameBufferRTV;
}

void FGraphicsDevice::ReleaseFrameBuffer()
{
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    if (FrameBufferRTV)
    {
        FrameBufferRTV->Release();
        FrameBufferRTV = nullptr;
    }

    if (UUIDFrameBuffer)
    {
        UUIDFrameBuffer->Release();
        UUIDFrameBuffer = nullptr;
    }

    if (UUIDFrameBufferRTV)
    {
        UUIDFrameBufferRTV->Release();
        UUIDFrameBufferRTV = nullptr;
    }

    if (SceneColorBuffer)
    {
        SceneColorBuffer->Release();
        SceneColorBuffer = nullptr;
    }

    if (SceneColorRTV)
    {
        SceneColorRTV->Release();
        SceneColorRTV = nullptr;
    }
}

void FGraphicsDevice::ReleaseRasterizerState()
{
    if (RasterizerStateSOLID)
    {
        RasterizerStateSOLID->Release();
        RasterizerStateSOLID = nullptr;
    }
    if (RasterizerStateWIREFRAME)
    {
        RasterizerStateWIREFRAME->Release();
        RasterizerStateWIREFRAME = nullptr;
    }
}

void FGraphicsDevice::ReleaseDepthStencilResources()
{
    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }

    // 깊이/스텐실 버퍼 해제
    if (DepthStencilBuffer)
    {
        DepthStencilBuffer->Release();
        DepthStencilBuffer = nullptr;
    }

    // 깊이/스텐실 상태 해제
    if (DepthStencilState)
    {
        DepthStencilState->Release();
        DepthStencilState = nullptr;
    }
    // 깊이 샘플러 해제
    if (DepthSampler)
    {
        DepthSampler->Release();
        DepthSampler = nullptr;
    }

    // 깊이 버퍼 SRV 해제
    if (DepthBufferSRV)
    {
        DepthBufferSRV->Release();
        DepthBufferSRV = nullptr;
    }

    if (DepthStateDisable)
    {
        DepthStateDisable->Release();
        DepthStateDisable = nullptr;
    }
}

void FGraphicsDevice::Release()
{
    ReleaseRasterizerState();
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();
    ReleaseDepthStencilResources();
    ReleaseDeviceAndSwapChain();
}

void FGraphicsDevice::SwapBuffer() const
{
    SwapChain->Present(1, 0);
}

void FGraphicsDevice::Prepare(const std::shared_ptr<FEditorViewportClient>& ActiveViewport) const
{
    Prepare();
    //TODO: 다른 곳으로 빼자
    TArray<UHeightFogComponent*> Fogs;
    for (const auto iter : TObjectRange<UHeightFogComponent>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld)
        {
            Fogs.Add(iter);
        }
    }
    if ((ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Fog)) && Fogs.Num() > 0)
        PrepareTexture();
}

void FGraphicsDevice::Prepare() const
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
    DeviceContext->ClearRenderTargetView(SceneColorRTV, ClearColor);                                         // 렌더 타겟 뷰에 저장된 이전 프레임 데이터를 삭제
    DeviceContext->ClearRenderTargetView(UUIDFrameBufferRTV, ClearColor);                                     // 렌더 타겟 뷰에 저장된 이전 프레임 데이터를 삭제
    
    //DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // 깊이 버퍼 초기화 추가

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정정 연결 방식 설정

    //DeviceContext->RSSetViewports(1, &ViewportInfo); // GPU가 화면을 렌더링할 영역 설정
    DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정

    DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

    DeviceContext->OMSetRenderTargets(2, RTVs, DepthStencilView); // 렌더 타겟 설정(백버퍼를 가르킴)
    float blendFactor[4] = { 0, 0, 0, 0 };
    DeviceContext->OMSetBlendState(AlphaBlendState, blendFactor, 0xffffffff);

}

void FGraphicsDevice::Prepare(D3D11_VIEWPORT* viewport) const
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);                                         // 렌더 타겟 뷰에 저장된 이전 프레임 데이터를 삭제
    //DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // 깊이 버퍼 초기화 추가

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정정 연결 방식 설정

    DeviceContext->RSSetViewports(1, viewport);   // GPU가 화면을 렌더링할 영역 설정
    DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정

    DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView); // 렌더 타겟 설정(백버퍼를 가르킴)
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);            // 블렌뎅 상태 설정, 기본블렌딩 상태임
}

void FGraphicsDevice::PrepareTexture() const
{
    ID3D11RenderTargetView* tempRTVs[2] =
    {
        SceneColorRTV,
        UUIDFrameBufferRTV
    };   
    DeviceContext->OMSetRenderTargets(2, tempRTVs, DepthStencilView);
    DeviceContext->OMSetBlendState(AlphaBlendState, nullptr, 0xffffffff);
}

void FGraphicsDevice::OnResize(HWND hWindow)
{
    DeviceContext->OMSetRenderTargets(0, RTVs, nullptr);

    FrameBufferRTV->Release();
    FrameBufferRTV = nullptr;

    UUIDFrameBufferRTV->Release();
    UUIDFrameBufferRTV = nullptr;

    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }

    ReleaseFrameBuffer();


    if (screenWidth == 0 || screenHeight == 0)
    {
        MessageBox(hWindow, L"Invalid width or height for ResizeBuffers!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // SwapChain 크기 조정
    HRESULT hr;
    hr = SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0); // DXGI_FORMAT_B8G8R8A8_UNORM으로 시도
    if (FAILED(hr))
    {
        MessageBox(hWindow, L"failed", L"ResizeBuffers failed ", MB_ICONERROR | MB_OK);
        return;
    }

    SwapChain->GetDesc(&SwapchainDesc);
    screenWidth = SwapchainDesc.BufferDesc.Width;
    screenHeight = SwapchainDesc.BufferDesc.Height;

    CreateFrameBuffer();
    CreateDepthStencilBuffer(hWindow);

    NotifyResizeEvents(screenWidth, screenHeight);
}
void FGraphicsDevice::CreateAlphaBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = Device->CreateBlendState(&blendDesc, &AlphaBlendState);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"AlphaBlendState 생성에 실패했습니다!", L"Error", MB_ICONERROR | MB_OK);
    }
}


void FGraphicsDevice::ChangeRasterizer(EViewModeIndex evi)
{
    switch (evi)
    {
    case EViewModeIndex::VMI_Wireframe:
        CurrentRasterizer = RasterizerStateWIREFRAME;
        break;
    case EViewModeIndex::VMI_Lit_Gouraud:
    case EViewModeIndex::VMI_Lit_Lambert:
    case EViewModeIndex::VMI_Lit_Phong:
    case EViewModeIndex::VMI_Unlit:
    case EViewModeIndex::VMI_WorldNormal:
    case EViewModeIndex::VMI_SceneDepth:
        CurrentRasterizer = RasterizerStateSOLID;
        break;
    }
    DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정
}

void FGraphicsDevice::ChangeDepthStencilState(ID3D11DepthStencilState* newDetptStencil) const
{
    DeviceContext->OMSetDepthStencilState(newDetptStencil, 0);
}

void FGraphicsDevice::CreateRTV(ID3D11Texture2D*& OutTexture, ID3D11RenderTargetView*& OutRTV)
{
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = screenWidth;
    TextureDesc.Height = screenHeight;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;
    Device->CreateTexture2D(&TextureDesc, nullptr, &OutTexture);

    D3D11_RENDER_TARGET_VIEW_DESC FogRTVDesc = {};
    FogRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 색상 포맷
    FogRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(OutTexture, &FogRTVDesc, &OutRTV);
}

void FGraphicsDevice::UnbindDSV()
{
    // 깊이 스텐실 뷰 해제
    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);

}

void FGraphicsDevice::RestoreDSV()
{
    // 깊이 스텐실 뷰 복원
    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);
}


uint32 FGraphicsDevice::GetPixelUUID(POINT pt) const
{
    // pt.x 값 제한하기
    if (pt.x < 0)
    {
        pt.x = 0;
    }
    else if (pt.x > screenWidth)
    {
        pt.x = screenWidth;
    }

    // pt.y 값 제한하기
    if (pt.y < 0)
    {
        pt.y = 0;
    }
    else if (pt.y > screenHeight)
    {
        pt.y = screenHeight;
    }

    // 1. Staging 텍스처 생성 (1x1 픽셀)
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = 1; // 픽셀 1개만 복사
    stagingDesc.Height = 1;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 원본 텍스처 포맷과 동일
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* stagingTexture = nullptr;
    Device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);

    // 2. 복사할 영역 정의 (D3D11_BOX)
    D3D11_BOX srcBox;
    srcBox.left = static_cast<UINT>(pt.x);
    srcBox.right = srcBox.left + 1; // 1픽셀 너비
    srcBox.top = static_cast<UINT>(pt.y);
    srcBox.bottom = srcBox.top + 1; // 1픽셀 높이
    srcBox.front = 0;
    srcBox.back = 1;
    FVector4 UUIDColor{1, 1, 1, 1};

    if (stagingTexture == nullptr)
        return DecodeUUIDColor(UUIDColor);

    // 3. 특정 좌표만 복사
    DeviceContext->CopySubresourceRegion(
        stagingTexture,  // 대상 텍스처
        0,               // 대상 서브리소스
        0, 0, 0,         // 대상 좌표 (x, y, z)
        UUIDFrameBuffer, // 원본 텍스처
        0,               // 원본 서브리소스
        &srcBox          // 복사 영역
    );

    // 4. 데이터 매핑
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    DeviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);

    // 5. 픽셀 데이터 추출 (1x1 텍스처이므로 offset = 0)
    const BYTE* pixelData = static_cast<const BYTE*>(mapped.pData);

    if (pixelData)
    {
        UUIDColor.X = static_cast<float>(pixelData[0]); // R
        UUIDColor.Y = static_cast<float>(pixelData[1]); // G
        UUIDColor.Z = static_cast<float>(pixelData[2]); // B
        UUIDColor.W = static_cast<float>(pixelData[3]); // A
    }

    // 6. 매핑 해제 및 정리
    DeviceContext->Unmap(stagingTexture, 0);
    if (stagingTexture) stagingTexture->Release();
    stagingTexture = nullptr;

    return DecodeUUIDColor(UUIDColor);
}

uint32 FGraphicsDevice::DecodeUUIDColor(FVector4 UUIDColor) const
{
    const uint32_t W = static_cast<uint32_t>(UUIDColor.W) << 24;
    const uint32_t Z = static_cast<uint32_t>(UUIDColor.Z) << 16;
    const uint32_t Y = static_cast<uint32_t>(UUIDColor.Y) << 8;
    const uint32_t X = static_cast<uint32_t>(UUIDColor.X);

    return W | Z | Y | X;
}

void FGraphicsDevice::SubscribeResizeEvent(const OnResizeEvent& Event)
{
    ResizeEvents.push_back(Event);
}

void FGraphicsDevice::UnsubscribeResizeEvent(const OnResizeEvent& Event)
{
    // !TODO : 이벤트 제거 불가능
    //auto it = std::remove(ResizeEvents.begin(), ResizeEvents.end(), Event);
    //if (it != ResizeEvents.end())
    //{
    //    ResizeEvents.erase(it, ResizeEvents.end());
    //}
}

void FGraphicsDevice::NotifyResizeEvents(UINT Width, UINT Height)
{
    for (const auto& Event : ResizeEvents)
    {
        Event(Width, Height);
    }
}
