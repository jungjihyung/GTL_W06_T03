#include "EngineLoop.h"
#include "ImGuiManager.h"
#include "UnrealClient.h"
#include "World/World.h"
#include "Camera/CameraComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ViewportTypePanel.h"
#include "Slate/Widgets/Layout/SSplitter.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"
#include "D3D11RHI/GraphicDevice.h"

#include "Engine/EditorEngine.h"


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }
    int zDelta;
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            //UGraphicsDevice 객체의 OnResize 함수 호출
            if (FEngineLoop::GraphicDevice.SwapChain)
            {
                FEngineLoop::GraphicDevice.OnResize(hWnd);
            }
            for (int i = 0; i < 4; i++)
            {
                if (GEngineLoop.GetLevelEditor())
                {
                    if (GEngineLoop.GetLevelEditor()->GetViewports()[i])
                    {
                        GEngineLoop.GetLevelEditor()->GetViewports()[i]->ResizeViewport(FEngineLoop::GraphicDevice.SwapchainDesc);
                    }
                }
            }
        }
        Console::GetInstance().OnResize(hWnd);
        // ControlPanel::GetInstance().OnResize(hWnd);
        // PropertyPanel::GetInstance().OnResize(hWnd);
        // Outliner::GetInstance().OnResize(hWnd);
        // ViewModeDropdown::GetInstance().OnResize(hWnd);
        // ShowFlags::GetInstance().OnResize(hWnd);
        if (GEngineLoop.GetUnrealEditor())
        {
            GEngineLoop.GetUnrealEditor()->OnResize(hWnd);
        }
        ViewportTypePanel::GetInstance().OnResize(hWnd);
        break;
    case WM_MOUSEWHEEL:
        if (ImGui::GetIO().WantCaptureMouse)
            return 0;
        zDelta = GET_WHEEL_DELTA_WPARAM(wParam); // 휠 회전 값 (+120 / -120)
        if (GEngineLoop.GetLevelEditor())
        {
            if (GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->IsPerspective())
            {
                if (GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetIsOnRBMouseClick())
                {
                    GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->SetCameraSpeedScalar(
                        static_cast<float>(GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar() + zDelta * 0.01)
                    );
                }
                else
                {
                    GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->CameraMoveForward(zDelta * 0.1f);
                }
            }
            else
            {
                FEditorViewportClient::SetOthoSize(-zDelta * 0.01f);
            }
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

FGraphicsDevice FEngineLoop::GraphicDevice;
FRenderer FEngineLoop::Renderer;
UPrimitiveDrawBatch FEngineLoop::PrimitiveDrawBatch;
FResourceMgr FEngineLoop::ResourceManager;
uint32 FEngineLoop::TotalAllocationBytes = 0;
uint32 FEngineLoop::TotalAllocationCount = 0;

FEngineLoop::FEngineLoop()
    : hWnd(nullptr)
    , UIMgr(nullptr)
    , LevelEditor(nullptr)
    , UnrealEditor(nullptr)
{
}

int32 FEngineLoop::PreInit()
{
    return 0;
}

int32 FEngineLoop::Init(HINSTANCE hInstance)
{
    /* must be initialized before window. */
    WindowInit(hInstance);

    UnrealEditor = new UnrealEd();

    bufferManager = new FDXDBufferManager();

    UIMgr = new UImGuiManager;

    LevelEditor = new SLevelEditor();

    UnrealEditor->Initialize();

    GraphicDevice.Initialize(hWnd);

    bufferManager->Initialize(GraphicDevice.Device, GraphicDevice.DeviceContext);

    Renderer.Initialize(&GraphicDevice, bufferManager);

    PrimitiveDrawBatch.Initialize(&GraphicDevice, bufferManager);

    UIMgr->Initialize(hWnd, GraphicDevice.Device, GraphicDevice.DeviceContext);

    ResourceManager.Initialize(&Renderer, &GraphicDevice);

    LevelEditor->Initialize();

    GEngine = FObjectFactory::ConstructObject<UEditorEngine>(nullptr);
    GEngine->Init();

    return 0;
}


void FEngineLoop::Render() const
{
    GraphicDevice.Prepare(LevelEditor->GetActiveViewportClient());
    if (LevelEditor->IsMultiViewport())
    {
        std::shared_ptr<FEditorViewportClient> viewportClient = GetLevelEditor()->GetActiveViewportClient();
        for (int i = 0; i < 4; ++i)
        {
            LevelEditor->SetViewportClient(i);
            // graphicDevice.DeviceContext->RSSetViewports(1, &LevelEditor->GetViewports()[i]->GetD3DViewport());
            // graphicDevice.ChangeRasterizer(LevelEditor->GetActiveViewportClient()->GetViewMode());
            // renderer.ChangeViewMode(LevelEditor->GetActiveViewportClient()->GetViewMode());
            // renderer.PrepareShader();
            // renderer.UpdateLightBuffer();
            // RenderWorld();
            Renderer.PrepareRender();
            Renderer.Render(LevelEditor->GetActiveViewportClient());
        }
        GetLevelEditor()->SetViewportClient(viewportClient);
    }
    else
    {
        // graphicDevice.DeviceContext->RSSetViewports(1, &LevelEditor->GetActiveViewportClient()->GetD3DViewport());
        // graphicDevice.ChangeRasterizer(LevelEditor->GetActiveViewportClient()->GetViewMode());
        // renderer.ChangeViewMode(LevelEditor->GetActiveViewportClient()->GetViewMode());
        // renderer.PrepareShader();
        // renderer.UpdateLightBuffer();
        // RenderWorld();
        Renderer.PrepareRender();
        Renderer.Render(LevelEditor->GetActiveViewportClient());
    }
}

void FEngineLoop::Tick()
{
    LARGE_INTEGER frequency;
    const double targetFrameTime = 1000.0 / targetFPS; // 한 프레임의 목표 시간 (밀리초 단위)

    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;

    while (bIsExit == false)
    {
        QueryPerformanceCounter(&startTime);

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // 키보드 입력 메시지를 문자메시지로 변경
            DispatchMessage(&msg);  // 메시지를 WndProc에 전달

            if (msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
        }

        float DeltaTime = elapsedTime / 1000.f;

        Input();
        GEngine->Tick(DeltaTime);
        LevelEditor->Tick(DeltaTime);
        Render();
        UIMgr->BeginFrame();
        UnrealEditor->Render();

        Console::GetInstance().Draw();

        UIMgr->EndFrame();

        // Pending 처리된 오브젝트 제거
        GUObjectArray.ProcessPendingDestroyObjects();

        GraphicDevice.SwapBuffer();
        do
        {
            Sleep(0);
            QueryPerformanceCounter(&endTime);
            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.f / frequency.QuadPart;
        } while (elapsedTime < targetFrameTime);
    }
}

float FEngineLoop::GetAspectRatio(IDXGISwapChain* swapChain) const
{
    DXGI_SWAP_CHAIN_DESC desc;
    swapChain->GetDesc(&desc);
    return static_cast<float>(desc.BufferDesc.Width) / static_cast<float>(desc.BufferDesc.Height);
}

void FEngineLoop::Input()
{
    if (GetAsyncKeyState('M') & 0x8000)
    {
        if (!bTestInput)
        {
            bTestInput = true;
            if (LevelEditor->IsMultiViewport())
            {
                LevelEditor->OffMultiViewport();
            }
            else
                LevelEditor->OnMultiViewport();
        }
    }
    else
    {
        bTestInput = false;
    }
}

void FEngineLoop::Exit()
{
    LevelEditor->Release();
    UIMgr->Shutdown();
    delete UIMgr;
    ResourceManager.Release(&Renderer);
    Renderer.Release();
    GraphicDevice.Release();
}


void FEngineLoop::WindowInit(HINSTANCE hInstance)
{
    WCHAR WindowClass[] = L"JungleWindowClass";

    WCHAR Title[] = L"Game Tech Lab";

    WNDCLASSW wndclass{};
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.lpszClassName = WindowClass;

    RegisterClassW(&wndclass);

    hWnd = CreateWindowExW(
        0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 1000,
        nullptr, nullptr, hInstance, nullptr
    );
}
