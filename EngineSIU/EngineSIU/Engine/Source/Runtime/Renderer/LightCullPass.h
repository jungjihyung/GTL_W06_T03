#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"
#include "Define.h"

class FEditorViewportClient;
class FDXDShaderManager;
class FGraphicsDevice;
class FDXDShaderManager;

class FLightCullPass : public IRenderPass
{
    // IRenderPass을(를) 통해 상속됨
public:
    void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    void PrepareRender() override;
    void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    void ClearRenderArr() override;
    void CreateShader();

private:
    // 가시성 라이트 인덱스 버퍼와 UAV
    void CreateVisibleLightBuffer();
    void CreateVisibleLightUAV();
    void CreateVisibleLightSRV();
    // 라이트 인덱스 카운트 버퍼와 UAV
    void CreateLightIndexCountBuffer();
    void CreateLightIndexCountUAV();
    void CreateLightIndexCountSRV();
    
    UINT GetMaxTileCount() const;
private:
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;
};
