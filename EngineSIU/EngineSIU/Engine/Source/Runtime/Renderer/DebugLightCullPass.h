#pragma once
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"
#include "Define.h"

class FDXDBufferManager;
class FGraphicsDevice;
class FDXDShaderManager;
class UWorld;
class FEditorViewportClient;

class FDebugLightCullPass :
    public IRenderPass
{
    // IRenderPass을(를) 통해 상속됨
public:
    void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    void PrepareRender() override;
    void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    void ClearRenderArr() override;
    void CreateShader();

private:
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;

    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11VertexShader* DebugVertexShader = nullptr;
    ID3D11PixelShader* DebugPixelShader = nullptr;
};

