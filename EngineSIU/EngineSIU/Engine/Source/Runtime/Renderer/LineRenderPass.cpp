#include "LineRenderPass.h"

#include "D3D11RHI/DXDBufferManager.h"

#include "D3D11RHI/GraphicDevice.h"

#include "D3D11RHI/DXDShaderManager.h"

#include "RendererHelpers.h"

#include "Math/JungleMath.h"

#include "EngineLoop.h"

#include "UObject/UObjectIterator.h"

#include "UnrealEd/EditorViewportClient.h"

// 생성자/소멸자
FLineRenderPass::FLineRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , VertexLineShader(nullptr)
    , PixelLineShader(nullptr)
{
}

FLineRenderPass::~FLineRenderPass()
{
    // 셰이더 자원 해제
    FDXDBufferManager::SafeRelease(VertexLineShader);
    FDXDBufferManager::SafeRelease(PixelLineShader);
}

void FLineRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;
    CreateShader();
}

void FLineRenderPass::PrepareRender()
{
}

void FLineRenderPass::ClearRenderArr()
{
    // 필요에 따라 내부 배열을 초기화
}

void FLineRenderPass::CreateShader()
{

    HRESULT hr = ShaderManager->AddVertexShader(L"Shaders/ShaderLine.hlsl", "mainVS", EViewModeIndex::VMI_Unlit, LineVertexShaderKey);
   
    hr = ShaderManager->AddPixelShader(L"Shaders/ShaderLine.hlsl", "mainPS", EViewModeIndex::VMI_Unlit, LinePixelShaderKey);
}

void FLineRenderPass::PrepareLineShader()
{
    VertexLineShader = ShaderManager->GetVertexShaderByKey(LineVertexShaderKey);
    PixelLineShader = ShaderManager->GetPixelShaderByKey(LinePixelShaderKey);


    Graphics->DeviceContext->VSSetShader(VertexLineShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelLineShader, nullptr, 0);

    BufferManager->BindConstantBuffer(TEXT("FPerObjectConstantBuffer"), 0, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FPerObjectConstantBuffer"), 0, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer(TEXT("FCameraConstantBuffer"), 2, EShaderStage::Pixel);

    BufferManager->BindConstantBuffer(TEXT("FGridParameters"), 1, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FGridParameters"), 1, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer(TEXT("FPrimitiveCounts"), 3, EShaderStage::Vertex);

    FEngineLoop::PrimitiveDrawBatch.PrepareLineResources();
}

void FLineRenderPass::DrawLineBatch(const FLinePrimitiveBatchArgs& BatchArgs) const
{
    UINT stride = sizeof(FSimpleVertex);
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &BatchArgs.VertexBuffer, &stride, &offset);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    const UINT vertexCountPerInstance = 2;
    UINT instanceCount = BatchArgs.GridParam.NumGridLines + 3 +
        (BatchArgs.BoundingBoxCount * 12) +
        (BatchArgs.ConeCount * (2 * BatchArgs.ConeSegmentCount)) +
        (12 * BatchArgs.OBBCount) +
        (BatchArgs.SphereCount * (3 * BatchArgs.SphereSegmentCount));

    Graphics->DeviceContext->DrawInstanced(vertexCountPerInstance, instanceCount, 0, 0);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FLineRenderPass::ProcessLineRendering(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareLineShader();


    FEngineLoop::PrimitiveDrawBatch.InitializeGrid(Viewport->GetGridSize(), 5000);
    // 상수 버퍼 업데이트: Identity 모델, 기본 색상 등
    FMatrix MVP = RendererHelpers::CalculateMVP(FMatrix::Identity, Viewport->GetViewMatrix(), Viewport->GetProjectionMatrix());
    FMatrix NormalMatrix = RendererHelpers::CalculateNormalMatrix(FMatrix::Identity);
    FPerObjectConstantBuffer Data(MVP, NormalMatrix, FVector4(0, 0, 0, 0), false);
    FCameraConstantBuffer CameraData;

    CameraData.View = Viewport->GetViewMatrix();
    CameraData.Projection = Viewport->GetProjectionMatrix();
    CameraData.InvProjection = FMatrix::Inverse(Viewport->GetProjectionMatrix());
    CameraData.CameraPosition = Viewport->ViewTransformPerspective.GetLocation();
    CameraData.CameraNear = Viewport->nearPlane;
    CameraData.CameraFar = Viewport->farPlane;

    BufferManager->UpdateConstantBuffer(TEXT("FPerObjectConstantBuffer"), Data);

    BufferManager->UpdateConstantBuffer(TEXT("FCameraConstantBuffer"), CameraData);
    FLinePrimitiveBatchArgs BatchArgs;
    FEngineLoop::PrimitiveDrawBatch.PrepareBatch(BatchArgs);
    DrawLineBatch(BatchArgs);
    FEngineLoop::PrimitiveDrawBatch.RemoveArr();
}

void FLineRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // 라이트 디버그 뷰 모드일 경우 라인렌더링 하지 않는다
    if (Viewport->GetViewMode() == EViewModeIndex::VMI_LightDebug)
        return;

    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->FrameBufferRTV, Graphics->DepthStencilView);

    ProcessLineRendering(Viewport);
}
