#pragma once
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"

#include "Define.h"

class FDXDShaderManager;

class UWorld;

class UMaterial;

class FEditorViewportClient;

class UStaticMeshComponent;

struct FStaticMaterial;

class FStaticMeshRenderPass : public IRenderPass
{
public:
    FStaticMeshRenderPass();
    
    ~FStaticMeshRenderPass();
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    
    virtual void PrepareRender() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

    void PrepareRenderState() const;
    
    void UpdatePerObjectConstant(const FMatrix& Model, const FMatrix& View, const FMatrix& Projection, const FVector4& UUIDColor, bool Selected) const;
  
    void RenderPrimitive(OBJ::FStaticMeshRenderData* RenderData, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int SelectedSubMeshIndex) const;
    
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const;

    void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const;

    // Shader 관련 함수 (생성/해제 등)
    void CreateShader();
    void ReleaseShader();

    void SwitchShaderLightingMode(EViewModeIndex evi);
private:
    TArray<UStaticMeshComponent*> StaticMeshObjs;

    ID3D11VertexShader* VertexShader;
     
    ID3D11PixelShader* PixelShader;
    
    ID3D11InputLayout* InputLayout;
    
    uint32 Stride;

    FDXDBufferManager* BufferManager;
    
    FGraphicsDevice* Graphics;
    
    FDXDShaderManager* ShaderManager;
 
    size_t UnlitPixelShaderKey;
    size_t GouraudPixelShaderKey;
    size_t LambertPixelShaderKey;
    size_t PhongPixelShaderKey;

    size_t UnlitVertexShaderKey;
    size_t GouraudVertexShaderKey;
    size_t LambertVertexShaderKey;
    size_t PhongVertexShaderKey;

};
