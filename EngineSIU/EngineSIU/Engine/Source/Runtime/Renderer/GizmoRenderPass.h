#pragma once

#include "Define.h"
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"

class UGizmoBaseComponent;

class FDXDBufferManager;

class FGraphicsDevice;

class FDXDShaderManager;

class UWorld;

class FEditorViewportClient;

class FGizmoRenderPass : public IRenderPass
{
public:
    FGizmoRenderPass();
    virtual ~FGizmoRenderPass();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;

    virtual void PrepareRender() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

    void PrepareRenderState() const;
    // Gizmo 한 개 렌더링 함수
    void RenderGizmoComponent(UGizmoBaseComponent* GizmoComp, const std::shared_ptr<FEditorViewportClient>& Viewport, const UWorld* World);

    void CreateShader();
    void ReleaseShader();
private:
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;

    ID3D11VertexShader* VertexShader;

    ID3D11PixelShader* PixelShader;

    ID3D11InputLayout* InputLayout;

    uint32 Stride;
    size_t GizmoPixelShaderKey;
    size_t GizmoVertexShaderKey;
    // 수집된 Gizmo 객체 배열
    //TArray<UGizmoBaseComponent*> GizmoObjs;
};
