#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "EngineBaseTypes.h"

#include "Core/HAL/PlatformType.h"
#include "Core/Math/Vector4.h"
#include <functional>

class FEditorViewportClient;

class FGraphicsDevice {
public:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    ID3D11Texture2D* FrameBuffer = nullptr;
    ID3D11Texture2D* UUIDFrameBuffer = nullptr;
    ID3D11Texture2D* SceneColorBuffer = nullptr;
    ID3D11RenderTargetView* RTVs[2];
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;
    ID3D11RenderTargetView* UUIDFrameBufferRTV = nullptr;
    ID3D11RenderTargetView* SceneColorRTV = nullptr;
    ID3D11RasterizerState* RasterizerStateSOLID = nullptr;
    ID3D11RasterizerState* RasterizerStateWIREFRAME = nullptr;
    DXGI_SWAP_CHAIN_DESC SwapchainDesc;
    ID3D11BlendState* AlphaBlendState = nullptr;
    
    UINT screenWidth = 0;
    UINT screenHeight = 0;
    // Depth-Stencil 관련 변수
    ID3D11Texture2D* DepthStencilBuffer = nullptr;  // 깊이/스텐실 텍스처
    ID3D11DepthStencilView* DepthStencilView = nullptr;  // 깊이/스텐실 뷰
    ID3D11DepthStencilState* DepthStencilState = nullptr;
    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear) 할 때 사용할 색상(RGBA)


    // Depth
    ID3D11SamplerState* DepthSampler = nullptr;
    ID3D11ShaderResourceView* DepthBufferSRV = nullptr;
    ID3D11DepthStencilState* DepthStateDisable = nullptr;

    // lightcull
    ID3D11Buffer* VisibleLightBuffer = nullptr;
    ID3D11UnorderedAccessView* VisibleLightUAV = nullptr; // GPU에서 Light데이터를 읽어오기 위한 UAV
    ID3D11ShaderResourceView* VisibleLightSRV = nullptr; // ComputeShader에서 작성한 데이터를 PixelShader에 넘겨주기 위한 SRV

    ID3D11Buffer* LightIndexCountBuffer = nullptr;
    ID3D11UnorderedAccessView* LightIndexCountUAV = nullptr;
    ID3D11ShaderResourceView* LightIndexCountSRV = nullptr;

    // Light StructuredBuffer
    ID3D11Buffer* LightBuffer = nullptr;
    ID3D11ShaderResourceView* LightBufferSRV = nullptr;

    void Initialize(HWND hWindow);
    void CreateDeviceAndSwapChain(HWND hWindow);
    void CreateDepthStencilBuffer(HWND hWindow);
    void CreateDepthStencilState();
    void CreateRasterizerState();
    void ReleaseDeviceAndSwapChain();
    void CreateFrameBuffer();
    void ReleaseFrameBuffer();
    void ReleaseRasterizerState();
    void ReleaseDepthStencilResources();
    void Release();
    void SwapBuffer() const;
    void Prepare(const std::shared_ptr<FEditorViewportClient>& ActiveViewport) const;
    void Prepare() const;
    void Prepare(D3D11_VIEWPORT* viewport) const;
    void PrepareTexture() const;
    void OnResize(HWND hWindow);
    ID3D11RasterizerState* GetCurrentRasterizer() const { return CurrentRasterizer; }
    void CreateAlphaBlendState();
    void ChangeRasterizer(EViewModeIndex evi);
    void ChangeDepthStencilState(ID3D11DepthStencilState* newDetptStencil) const;

    void CreateRTV(ID3D11Texture2D*& OutTexture, ID3D11RenderTargetView*& OutRTV);

    // DepthSRV를 쉐이더에 바인딩하기 전/후에 호출
    void UnbindDSV();
    void RestoreDSV();

    uint32 GetPixelUUID(POINT pt) const;
    uint32 DecodeUUIDColor(FVector4 UUIDColor) const;
private:
    ID3D11RasterizerState* CurrentRasterizer = nullptr;

    // on resize event
public:
    using OnResizeEvent = std::function<void(UINT, UINT)>;
    void SubscribeResizeEvent(const OnResizeEvent& Event);
    void UnsubscribeResizeEvent(const OnResizeEvent& Event);

private:
    std::vector<OnResizeEvent> ResizeEvents;
    void NotifyResizeEvents(UINT Width, UINT Height);

};

