#pragma once
#pragma once
#include <memory>

class UWorld;
class FDXDBufferManager;
class FDXDShaderManager;
class FGraphicsDevice;
class FEditorViewportClient;

class IRenderPass {
public:
    virtual ~IRenderPass() {}
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) = 0;
    virtual void PrepareRender() = 0;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;
    virtual void ClearRenderArr() = 0;
};
