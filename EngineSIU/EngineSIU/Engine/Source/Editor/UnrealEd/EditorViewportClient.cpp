#include "EditorViewportClient.h"
#include "fstream"
#include "sstream"
#include "ostream"
#include "Math/JungleMath.h"
#include "UnrealClient.h"
#include "World/World.h"
#include "GameFramework/Actor.h"
#include "Engine/EditorEngine.h"

#include "UObject/ObjectFactory.h"
#include "BaseGizmos/TransformGizmo.h"

FVector FEditorViewportClient::Pivot = FVector(0.0f, 0.0f, 0.0f);
float FEditorViewportClient::orthoSize = 10.0f;

FEditorViewportClient::FEditorViewportClient()
    : Viewport(nullptr)
    , ViewportType(LVT_Perspective)
    , ShowFlag(31)
    , ViewMode(VMI_Lit_Phong)
{
}

FEditorViewportClient::~FEditorViewportClient()
{
    Release();
}

void FEditorViewportClient::Draw(FViewport* Viewport)
{
}

void FEditorViewportClient::Initialize(int32 viewportIndex)
{

    ViewTransformPerspective.SetLocation(FVector(8.0f, 8.0f, 8.f));
    ViewTransformPerspective.SetRotation(FVector(0.0f, 45.0f, -135.0f));
    Viewport = new FViewport(static_cast<EViewScreenLocation>(viewportIndex));
    ResizeViewport(FEngineLoop::GraphicDevice.SwapchainDesc);
    ViewportIndex = viewportIndex;

    GizmoActor = FObjectFactory::ConstructObject<ATransformGizmo>(GEngine); // TODO : EditorEngine 외의 다른 Engine 형태가 추가되면 GEngine 대신 다른 방식으로 넣어주어야 함.
    GizmoActor->Initialize(this);
}

void FEditorViewportClient::Tick(float DeltaTime)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
    GizmoActor->Tick(DeltaTime);
}

void FEditorViewportClient::Release() const
{
    delete Viewport;
}

void FEditorViewportClient::Input()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) // VK_RBUTTON은 마우스 오른쪽 버튼을 나타냄
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        if (!bRightMouseDown)
        {
            // 마우스 오른쪽 버튼을 처음 눌렀을 때, 마우스 위치 초기화
            GetCursorPos(&lastMousePos);
            bRightMouseDown = true;
        }
        else
        {
            // 마우스 이동량 계산
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);

            // 마우스 이동 차이 계산
            int32 DeltaX = currentMousePos.x - lastMousePos.x;
            int32 DeltaY = currentMousePos.y - lastMousePos.y;

            // Yaw(좌우 회전) 및 Pitch(상하 회전) 값 변경
            if (IsPerspective()) {
                CameraRotateYaw(DeltaX * 0.1f);  // X 이동에 따라 좌우 회전
                CameraRotatePitch(DeltaY * 0.1f);  // Y 이동에 따라 상하 회전
            }
            else
            {
                PivotMoveRight(DeltaX);
                PivotMoveUp(DeltaY);
            }

            SetCursorPos(lastMousePos.x, lastMousePos.y);
        }
        if (GetAsyncKeyState('A') & 0x8000)
        {
            CameraMoveRight(-1.f);
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            CameraMoveRight(1.f);
        }
        if (GetAsyncKeyState('W') & 0x8000)
        {
            CameraMoveForward(1.f);
        }
        if (GetAsyncKeyState('S') & 0x8000)
        {
            CameraMoveForward(-1.f);
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            CameraMoveUp(1.f);
        }
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            CameraMoveUp(-1.f);
        }
    }
    else
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        bRightMouseDown = false; // 마우스 오른쪽 버튼을 떼면 상태 초기화
    }

    // Focus Selected Actor
    if (GetAsyncKeyState('F') & 0x8000)
    {
        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        if (AActor* PickedActor = Engine->GetSelectedActor())
        {
            FViewportCameraTransform& ViewTransform = ViewTransformPerspective;
            ViewTransform.SetLocation(
                // TODO: 10.0f 대신, 정점의 min, max의 거리를 구해서 하면 좋을 듯
                PickedActor->GetActorLocation() - (ViewTransform.GetForwardVector() * 10.0f)
            );
        }
    }
}
void FEditorViewportClient::ResizeViewport(const DXGI_SWAP_CHAIN_DESC& swapchaindesc)
{
    if (Viewport) { 
        Viewport->ResizeViewport(swapchaindesc);    
    }
    else {
        UE_LOG(LogLevel::Error, "Viewport is nullptr");
    }
    AspectRatio = GEngineLoop.GetAspectRatio(FEngineLoop::GraphicDevice.SwapChain);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}
void FEditorViewportClient::ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right)
{
    if (Viewport) {
        Viewport->ResizeViewport(Top, Bottom, Left, Right);
    }
    else {
        UE_LOG(LogLevel::Error, "Viewport is nullptr");
    }
    AspectRatio = GEngineLoop.GetAspectRatio(FEngineLoop::GraphicDevice.SwapChain);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}
bool FEditorViewportClient::IsSelected(POINT InPoint) const
{
    float TopLeftX = Viewport->GetViewport().TopLeftX;
    float TopLeftY = Viewport->GetViewport().TopLeftY;
    float Width = Viewport->GetViewport().Width;
    float Height = Viewport->GetViewport().Height;

    if (InPoint.x >= TopLeftX && InPoint.x <= TopLeftX + Width &&
        InPoint.y >= TopLeftY && InPoint.y <= TopLeftY + Height)
    {
        return true;
    }
    return false;
}
D3D11_VIEWPORT& FEditorViewportClient::GetD3DViewport() const
{
    return Viewport->GetViewport();
}
void FEditorViewportClient::CameraMoveForward(float InValue)
{
    if (IsPerspective()) {
        FVector curCameraLoc = ViewTransformPerspective.GetLocation();
        curCameraLoc = curCameraLoc + ViewTransformPerspective.GetForwardVector() * GetCameraSpeedScalar() * InValue;
        ViewTransformPerspective.SetLocation(curCameraLoc);
    }
    else
    {
        Pivot.X += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraMoveRight(float InValue)
{
    if (IsPerspective()) {
        FVector curCameraLoc = ViewTransformPerspective.GetLocation();
        curCameraLoc = curCameraLoc + ViewTransformPerspective.GetRightVector() * GetCameraSpeedScalar() * InValue;
        ViewTransformPerspective.SetLocation(curCameraLoc);
    }
    else
    {
        Pivot.Y += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraMoveUp(float InValue)
{
    if (IsPerspective()) {
        FVector curCameraLoc = ViewTransformPerspective.GetLocation();
        curCameraLoc.Z = curCameraLoc.Z + GetCameraSpeedScalar() * InValue;
        ViewTransformPerspective.SetLocation(curCameraLoc);
    }
    else {
        Pivot.Z += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraRotateYaw(float InValue)
{
    FVector curCameraRot = ViewTransformPerspective.GetRotation();
    curCameraRot.Z += InValue ;
    ViewTransformPerspective.SetRotation(curCameraRot);
}

void FEditorViewportClient::CameraRotatePitch(float InValue)
{
    FVector curCameraRot = ViewTransformPerspective.GetRotation();
    curCameraRot.Y = FMath::Clamp(curCameraRot.Y + InValue, -89.f, 89.f);
    ViewTransformPerspective.SetRotation(curCameraRot);
}

void FEditorViewportClient::PivotMoveRight(float InValue)
{
    Pivot = Pivot + ViewTransformOrthographic.GetRightVector() * InValue * -0.05f;
}

void FEditorViewportClient::PivotMoveUp(float InValue)
{
    Pivot = Pivot + ViewTransformOrthographic.GetUpVector() * InValue * 0.05f;
}

void FEditorViewportClient::UpdateViewMatrix()
{
    if (IsPerspective()) {
        View = JungleMath::CreateViewMatrix(ViewTransformPerspective.GetLocation(),
            ViewTransformPerspective.GetLocation() + ViewTransformPerspective.GetForwardVector(),
            FVector{ 0.0f,0.0f, 1.0f });
    }
    else 
    {
        UpdateOrthoCameraLoc();
        if (ViewportType == LVT_OrthoXY || ViewportType == LVT_OrthoNegativeXY) {
            View = JungleMath::CreateViewMatrix(ViewTransformOrthographic.GetLocation(),
                Pivot, FVector(0.0f, -1.0f, 0.0f));
        }
        else
        {
            View = JungleMath::CreateViewMatrix(ViewTransformOrthographic.GetLocation(),
                Pivot, FVector(0.0f, 0.0f, 1.0f));
        }
    }
}

void FEditorViewportClient::UpdateProjectionMatrix()
{
    if (IsPerspective()) {
        Projection = JungleMath::CreateProjectionMatrix(
            ViewFOV * (PI / 180.0f),
            GetViewport()->GetViewport().Width/ GetViewport()->GetViewport().Height,
            nearPlane,
            farPlane
        );
    }
    else
    {
        // 스왑체인의 가로세로 비율을 구합니다.
        float aspectRatio = GetViewport()->GetViewport().Width / GetViewport()->GetViewport().Height;

        // 오쏘그래픽 너비는 줌 값과 가로세로 비율에 따라 결정됩니다.
        float orthoWidth = orthoSize * aspectRatio;
        float orthoHeight = orthoSize;

        // 오쏘그래픽 투영 행렬 생성 (nearPlane, farPlane 은 기존 값 사용)
        Projection = JungleMath::CreateOrthoProjectionMatrix(
            orthoWidth,
            orthoHeight,
            nearPlane,
            farPlane
        );
    }
}

bool FEditorViewportClient::IsOrtho() const
{
    return !IsPerspective();
}

bool FEditorViewportClient::IsPerspective() const
{
    return (GetViewportType() == LVT_Perspective);
}

ELevelViewportType FEditorViewportClient::GetViewportType() const
{
    ELevelViewportType EffectiveViewportType = ViewportType;
    if (EffectiveViewportType == LVT_None)
    {
        EffectiveViewportType = LVT_Perspective;
    }
    //if (bUseControllingActorViewInfo)
    //{
    //    EffectiveViewportType = (ControllingActorViewInfo.ProjectionMode == ECameraProjectionMode::Perspective) ? LVT_Perspective : LVT_OrthoFreelook;
    //}
    return EffectiveViewportType;
}

void FEditorViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
    ViewportType = InViewportType;
    //ApplyViewMode(GetViewMode(), IsPerspective(), EngineShowFlags);

    //// We might have changed to an orthographic viewport; if so, update any viewport links
    //UpdateLinkedOrthoViewports(true);

    //Invalidate();
}

void FEditorViewportClient::UpdateOrthoCameraLoc()
{
    switch (ViewportType)
    {
    case LVT_OrthoXY: // Top
        ViewTransformOrthographic.SetLocation(Pivot + FVector::UpVector*100000.0f);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 90.0f, -90.0f));
        break;
    case LVT_OrthoXZ: // Front
        ViewTransformOrthographic.SetLocation(Pivot + FVector::ForwardVector*100000.0f);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 180.0f));
        break;
    case LVT_OrthoYZ: // Left
        ViewTransformOrthographic.SetLocation(Pivot + FVector::RightVector*100000.0f);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 270.0f));
        break;
    case LVT_Perspective:
        break;
    case LVT_OrthoNegativeXY: // Bottom
        ViewTransformOrthographic.SetLocation(Pivot + FVector::UpVector * -1.0f*100000.0f);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, -90.0f, 90.0f));
        break;
    case LVT_OrthoNegativeXZ: // Back
        ViewTransformOrthographic.SetLocation(Pivot + FVector::ForwardVector * -1.0f*100000.0f);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 0.0f));
        break;
    case LVT_OrthoNegativeYZ: // Right
        ViewTransformOrthographic.SetLocation(Pivot + FVector::RightVector * -1.0f*100000.0f);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 90.0f));
        break;
    case LVT_MAX:
        break;
    case LVT_None:
        break;
    default:
        break;
    }
}

void FEditorViewportClient::SetOthoSize(float InValue)
{
    orthoSize += InValue;
    if (orthoSize <= 0.1f)
        orthoSize = 0.1f;
    
}

void FEditorViewportClient::LoadConfig(const TMap<FString, FString>& config)
{
    FString ViewportNum = std::to_string(ViewportIndex);
    CameraSpeedSetting = GetValueFromConfig(config, "CameraSpeedSetting" + ViewportNum, 1);
    CameraSpeedScalar = GetValueFromConfig(config, "CameraSpeedScalar" + ViewportNum, 1.0f);
    GridSize = GetValueFromConfig(config, "GridSize"+ ViewportNum, 10.0f);
    ViewTransformPerspective.ViewLocation.X = GetValueFromConfig(config, "PerspectiveCameraLocX" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewLocation.Y = GetValueFromConfig(config, "PerspectiveCameraLocY" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewLocation.Z = GetValueFromConfig(config, "PerspectiveCameraLocZ" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewRotation.X = GetValueFromConfig(config, "PerspectiveCameraRotX" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewRotation.Y = GetValueFromConfig(config, "PerspectiveCameraRotY" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewRotation.Z = GetValueFromConfig(config, "PerspectiveCameraRotZ" + ViewportNum, 0.0f);
    ShowFlag = GetValueFromConfig(config, "ShowFlag" + ViewportNum, 31.0f);
    ViewMode = static_cast<EViewModeIndex>(GetValueFromConfig(config, "ViewMode" + ViewportNum, 0));
    ViewportType = static_cast<ELevelViewportType>(GetValueFromConfig(config, "ViewportType" + ViewportNum, 3));
}
void FEditorViewportClient::SaveConfig(TMap<FString, FString>& config) const
{
    FString ViewportNum = std::to_string(ViewportIndex);
    config["CameraSpeedSetting"+ ViewportNum] = std::to_string(CameraSpeedSetting);
    config["CameraSpeedScalar"+ ViewportNum] = std::to_string(CameraSpeedScalar);
    config["GridSize"+ ViewportNum] = std::to_string(GridSize);
    config["PerspectiveCameraLocX" + ViewportNum] = std::to_string(ViewTransformPerspective.GetLocation().X);
    config["PerspectiveCameraLocY" + ViewportNum] = std::to_string(ViewTransformPerspective.GetLocation().Y);
    config["PerspectiveCameraLocZ" + ViewportNum] = std::to_string(ViewTransformPerspective.GetLocation().Z);
    config["PerspectiveCameraRotX" + ViewportNum] = std::to_string(ViewTransformPerspective.GetRotation().X);
    config["PerspectiveCameraRotY" + ViewportNum] = std::to_string(ViewTransformPerspective.GetRotation().Y);
    config["PerspectiveCameraRotZ" + ViewportNum] = std::to_string(ViewTransformPerspective.GetRotation().Z);
    config["ShowFlag"+ ViewportNum] = std::to_string(ShowFlag);
    config["ViewMode" + ViewportNum] = std::to_string(int32(ViewMode));
    config["ViewportType" + ViewportNum] = std::to_string(int32(ViewportType));
}
TMap<FString, FString> FEditorViewportClient::ReadIniFile(const FString& filePath) const
{
    TMap<FString, FString> config;
    std::ifstream file(*filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == ';') continue;
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            config[key] = value;
        }
    }
    return config;
}

void FEditorViewportClient::WriteIniFile(const FString& filePath, const TMap<FString, FString>& config) const
{
    std::ofstream file(*filePath);
    for (const auto& pair : config) {
        file << *pair.Key << "=" << *pair.Value << "\n";
    }
}

void FEditorViewportClient::SetCameraSpeedScalar(float value)
{
    if (value < 0.198f)
        value = 0.198f;
    else if (value > 176.0f)
        value = 176.0f;
    CameraSpeedScalar = value;
}


FVector FViewportCameraTransform::GetForwardVector() const
{
    FVector Forward = FVector(1.f, 0.f, 0.0f);
    Forward = JungleMath::FVectorRotate(Forward, ViewRotation);
    return Forward;
}
FVector FViewportCameraTransform::GetRightVector() const
{
    FVector Right = FVector(0.f, 1.f, 0.0f);
	Right = JungleMath::FVectorRotate(Right, ViewRotation);
	return Right;
}

FVector FViewportCameraTransform::GetUpVector() const
{
    FVector Up = FVector(0.f, 0.f, 1.0f);
    Up = JungleMath::FVectorRotate(Up, ViewRotation);
    return Up;
}
