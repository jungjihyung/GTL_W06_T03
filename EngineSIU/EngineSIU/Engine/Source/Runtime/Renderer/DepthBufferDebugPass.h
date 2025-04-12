#pragma once
#define _TCHAR_DEFINED
#include <d3d11.h>
#include <memory>
class FGraphicsDevice;
class FDXDShaderManager;
class FDXDBufferManager;
class FEditorViewportClient;

class FDepthBufferDebugPass
{
public:
    FDepthBufferDebugPass();
    ~FDepthBufferDebugPass();

    // 초기화: 그래픽 디바이스와 셰이더 매니저를 등록
    void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager);

    // 스프라이트 렌더링에 필요한 버퍼 및 상태 생성
    void CreateSpriteResources();

    // Depth Buffer 렌더링용 셰이더 생성 및 입력 레이아웃 설정
    void CreateShader();

    void CreateDepthBufferSrv();
  
    void PrepareRenderState();
    // Depth Buffer를 화면에 렌더링
    void RenderDepthBuffer(const std::shared_ptr<FEditorViewportClient>& ActiveViewport);

    void UpdateDepthBufferSRV();

    void UpdateScreenConstant(const D3D11_VIEWPORT& viewport);

    ID3D11ShaderResourceView* GetDepthSRV() { return DepthBufferSRV; }

private:
    ID3D11SamplerState* DepthSampler = nullptr;

    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FDXDShaderManager* ShaderManager;

    
    // Depth Buffer 렌더링용 셰이더 및 입력 레이아웃
    ID3D11VertexShader* SpriteVertexShader;
    ID3D11PixelShader* DepthBufferPixelShader;
    ID3D11InputLayout* InputLayout;

    size_t DepthBufferPixelShaderKey;
    size_t DepthBufferVertexShaderKey;

    // Depth Buffer SRV (외부에서 등록)
    ID3D11ShaderResourceView* DepthBufferSRV;

    // Depth Write 비활성화 상태 (Depth Buffer 렌더링용)
    ID3D11DepthStencilState* DepthStateDisable;
    bool bRender = false;
    float screenWidth = 0;
    float screenHeight = 0;
};
