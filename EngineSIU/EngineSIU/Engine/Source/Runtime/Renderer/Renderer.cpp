
#include "Renderer.h"
#include "World/World.h"
#include "Engine/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "RendererHelpers.h"
#include "StaticMeshRenderPass.h"
#include "BillboardRenderPass.h"
#include "GizmoRenderPass.h"
#include "UpdateLightBufferPass.h"
#include "LineRenderPass.h"
#include "DepthBufferDebugPass.h"
#include "FogRenderPass.h"
#include "LightCullPass.h"
#include <UObject/UObjectIterator.h>
#include <UObject/Casts.h>
#include "GameFrameWork/Actor.h"

DWORD WINAPI DirectoryChangeWatcher(LPVOID lpParam)
{
    FWatchParams* pParams = reinterpret_cast<FWatchParams*>(lpParam);
    FDXDShaderManager* pShaderManager = pParams->pShaderManager;
    std::wstring directory = pParams->Directory;
    delete pParams;  // 동적 할당된 파라미터 메모리 해제

    // 디렉터리 감시 핸들 생성 (디렉터리 접근 시 FILE_FLAG_BACKUP_SEMANTICS 필요)
    HANDLE hDir = CreateFileW(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    if (hDir == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringA("디렉토리 감시 핸들 생성 실패\n");
        return 1;
    }

    const DWORD bufferSize = 4096;
    BYTE buffer[bufferSize];
    DWORD bytesReturned = 0;

    OVERLAPPED overlapped = {};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    while (true)
    {
        BOOL success = ReadDirectoryChangesW(
            hDir,
            buffer,
            bufferSize,
            TRUE,
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            &overlapped,
            NULL);
        if (success)
        {
            DWORD waitStatus = WaitForSingleObject(overlapped.hEvent, 1000);
            if (waitStatus == WAIT_OBJECT_0)
            {
                pShaderManager->CheckAndReloadShaders();
                ResetEvent(overlapped.hEvent);
            }
        }
        Sleep(50);  // 너무 빠른 루프 방지
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(hDir);
    return 0;
}

//------------------------------------------------------------------------------
// 초기화 및 해제 관련 함수
//------------------------------------------------------------------------------
void FRenderer::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;

    ShaderManager = new FDXDShaderManager(Graphics->Device, Graphics);
    StaticMeshRenderPass = new FStaticMeshRenderPass();
    BillboardRenderPass = new FBillboardRenderPass();
    GizmoRenderPass = new FGizmoRenderPass();
    UpdateLightBufferPass = new FUpdateLightBufferPass();
    LineRenderPass = new FLineRenderPass();
    DepthBufferDebugPass = new FDepthBufferDebugPass();
    FogRenderPass = new FFogRenderPass();
    LightCullPass = new FLightCullPass();

    StaticMeshRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    BillboardRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    GizmoRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    UpdateLightBufferPass->Initialize(BufferManager, Graphics, ShaderManager);
    LineRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    DepthBufferDebugPass->Initialize(BufferManager, Graphics, ShaderManager);
    FogRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    LightCullPass->Initialize(BufferManager, Graphics, ShaderManager);

    CreateConstantBuffers();

    FWatchParams* pParams = new FWatchParams();
    pParams->pShaderManager = ShaderManager;  // 이미 생성된 FDXDShaderManager 인스턴스
    pParams->Directory = L"Shaders";


    HANDLE hThread = CreateThread(
        nullptr,        // 기본 보안 속성
        0,              // 기본 스택 크기
        DirectoryChangeWatcher, // 스레드 시작 주소
        pParams,        // FWatchParams 포인터 전달
        0,              // 즉시 실행
        nullptr         // 스레드 ID 무시
    );
}

void FRenderer::Release()
{
}


void FRenderer::ChangeViewMode(EViewModeIndex evi)
{
    if (evi == EViewModeIndex::VMI_SceneDepth)
        IsSceneDepth = true;
    else
        IsSceneDepth = false;
 
}

//------------------------------------------------------------------------------
// 사용하는 모든 상수 버퍼 생성
//------------------------------------------------------------------------------
void FRenderer::CreateConstantBuffers()
{
    UINT perObjectBufferSize = sizeof(FPerObjectConstantBuffer);
    BufferManager->CreateBufferGeneric<FPerObjectConstantBuffer>("FPerObjectConstantBuffer", nullptr, perObjectBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT cameraConstantBufferSize = sizeof(FCameraConstantBuffer);
    BufferManager->CreateBufferGeneric<FCameraConstantBuffer>("FCameraConstantBuffer", nullptr, cameraConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT subUVBufferSize = sizeof(FSubUVConstant);
    BufferManager->CreateBufferGeneric<FSubUVConstant>("FSubUVConstant", nullptr, subUVBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT materialBufferSize = sizeof(FMaterialConstants);
    BufferManager->CreateBufferGeneric<FMaterialConstants>("FMaterialConstants", nullptr, materialBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT subMeshBufferSize = sizeof(FSubMeshConstants);
    BufferManager->CreateBufferGeneric<FSubMeshConstants>("FSubMeshConstants", nullptr, subMeshBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT textureBufferSize = sizeof(FTextureConstants);
    BufferManager->CreateBufferGeneric<FTextureConstants>("FTextureConstants", nullptr, textureBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT lightingBufferSize = sizeof(FLightBuffer);
    BufferManager->CreateBufferGeneric<FLightBuffer>("FLightBuffer", nullptr, lightingBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ScreenConstantsBufferSize = sizeof(FScreenConstants);
    BufferManager->CreateBufferGeneric<FScreenConstants>("FScreenConstants", nullptr, ScreenConstantsBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT FogConstantBufferSize = sizeof(FFogConstants);
    BufferManager->CreateBufferGeneric<FFogConstants>("FFogConstants", nullptr, FogConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    
    UINT FPrimitiveCountsSize = sizeof(FPrimitiveCounts);
    BufferManager->CreateBufferGeneric<FPrimitiveCounts>("FPrimitiveCounts", nullptr, FPrimitiveCountsSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT FGridParametersSize = sizeof(FGridParameters);
    BufferManager->CreateBufferGeneric<FGridParameters>("FGridParameters", nullptr, FGridParametersSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FRenderer::ReleaseConstantBuffer()
{
    BufferManager->ReleaseConstantBuffer();
}

void FRenderer::PrepareRender()
{
    StaticMeshRenderPass->PrepareRenderState();
    StaticMeshRenderPass->PrepareRender();
    GizmoRenderPass->PrepareRender();
    BillboardRenderPass->PrepareRender();
    UpdateLightBufferPass->PrepareRender();
    FogRenderPass->PrepareRender();
}

void FRenderer::ClearRenderArr()
{
    StaticMeshRenderPass->ClearRenderArr();
    BillboardRenderPass->ClearRenderArr();
    GizmoRenderPass->ClearRenderArr();
    UpdateLightBufferPass->ClearRenderArr();
    FogRenderPass->ClearRenderArr();
}

void FRenderer::Render(const std::shared_ptr<FEditorViewportClient>& ActiveViewport)
{
    Graphics->DeviceContext->RSSetViewports(1, &ActiveViewport->GetD3DViewport());


    Graphics->ChangeRasterizer(ActiveViewport->GetViewMode());

    ChangeViewMode(ActiveViewport->GetViewMode());

    UpdateLightBufferPass->Render(ActiveViewport);
   
    // !TODO : LightCullPass->Render
    LightCullPass->Render(ActiveViewport);

    StaticMeshRenderPass->SwitchShaderLightingMode(ActiveViewport->GetViewMode());
    StaticMeshRenderPass->Render(ActiveViewport);
    BillboardRenderPass->Render(ActiveViewport);
    

    if (IsSceneDepth)
    {
        DepthBufferDebugPass->RenderDepthBuffer(ActiveViewport);
    }

    if (!IsSceneDepth)
    {
        DepthBufferDebugPass->UpdateDepthBufferSRV();
        
        FogRenderPass->RenderFog(ActiveViewport, Graphics->DepthBufferSRV);
    }
    LineRenderPass->Render(ActiveViewport);



    GizmoRenderPass->Render(ActiveViewport);



    ClearRenderArr();
}
