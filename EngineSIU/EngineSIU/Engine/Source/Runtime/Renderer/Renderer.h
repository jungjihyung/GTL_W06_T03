#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>
#include <d3dcompiler.h>

#include "EngineBaseTypes.h"
#include "Define.h"
#include "Container/Set.h"

#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/HotReload/ShaderHotReload.h"


class UWorld;
class UObject;

class FDXDShaderManager;
class FEditorViewportClient;

class FStaticMeshRenderPass;
class FBillboardRenderPass;
class FGizmoRenderPass;
class FUpdateLightBufferPass;
class FDepthBufferDebugPass;
class FLineRenderPass;
class FFogRenderPass;
class FLightCullPass;
class FDebugLightCullPass;

class FRenderer
{
public:
    //==========================================================================
    // 초기화/해제 관련 함수
    //==========================================================================
    void Initialize(FGraphicsDevice* graphics, FDXDBufferManager* bufferManager);
    void Release();

    //==========================================================================
    // 렌더 패스 관련 함수
    //==========================================================================
    void PrepareRender();
    void ClearRenderArr();
    void Render(const std::shared_ptr<FEditorViewportClient>& ActiveViewport);

    // 뷰 모드 변경
    void ChangeViewMode(EViewModeIndex evi);
    //==========================================================================
    // 버퍼 생성/해제 함수 (템플릿 포함)
    //==========================================================================
public:
    template<typename T>
    ID3D11Buffer* CreateImmutableVertexBuffer(const FString& key, const TArray<T>& Vertices);

    ID3D11Buffer* CreateImmutableIndexBuffer(const FString& key, const TArray<uint32>& indices);

    // 상수 버퍼 생성/해제
    void CreateConstantBuffers();
    void ReleaseConstantBuffer();

public:
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FDXDShaderManager* ShaderManager = nullptr;

    FLightCullPass* LightCullPass = nullptr;
    FStaticMeshRenderPass* StaticMeshRenderPass = nullptr;
    FBillboardRenderPass* BillboardRenderPass = nullptr;
    FGizmoRenderPass* GizmoRenderPass = nullptr;
    FUpdateLightBufferPass* UpdateLightBufferPass = nullptr;
    FLineRenderPass* LineRenderPass = nullptr;
    FDepthBufferDebugPass* DepthBufferDebugPass = nullptr;
    FFogRenderPass* FogRenderPass = nullptr;
    FDebugLightCullPass* DebugLightCullPass = nullptr;

    FShaderHotReload* ShaderHotReload = nullptr;

    bool IsSceneDepth = false;
};

template<typename T>
inline ID3D11Buffer* FRenderer::CreateImmutableVertexBuffer(const FString& key, const TArray<T>& Vertices)
{
    FVertexInfo VertexBufferInfo;
    BufferManager->CreateVertexBuffer(key, Vertices, VertexBufferInfo);
    return VertexBufferInfo.VertexBuffer;
}

inline ID3D11Buffer* FRenderer::CreateImmutableIndexBuffer(const FString& key, const TArray<uint32>& indices)
{
    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(key, indices, IndexInfo);
    return IndexInfo.IndexBuffer;
}
