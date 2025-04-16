#pragma once
#include <sstream>

#include "Define.h"
#include "Container/Map.h"
#include "UObject/ObjectMacros.h"
#include "ViewportClient.h"
#include "EngineLoop.h"
#include "EngineBaseTypes.h"

#define MIN_ORTHOZOOM				1.0							/* 2D ortho viewport zoom >= MIN_ORTHOZOOM */
#define MAX_ORTHOZOOM				1e25

class ATransformGizmo;
class USceneComponent;

struct FViewportCameraTransform
{
public:
    FViewportCameraTransform() = default;

    /** Sets the transform's location */
    void SetLocation(const FVector& Position)
    {
        ViewLocation = Position;
    }

    /** Sets the transform's rotation */
    void SetRotation(const FVector& Rotation)
    {
        ViewRotation = Rotation;
    }

    /** Sets the location to look at during orbit */
    void SetLookAt(const FVector& InLookAt)
    {
        LookAt = InLookAt;
    }

    /** Set the ortho zoom amount */
    void SetOrthoZoom(float InOrthoZoom)
    {
        assert(InOrthoZoom >= MIN_ORTHOZOOM && InOrthoZoom <= MAX_ORTHOZOOM);
        OrthoZoom = InOrthoZoom;
    }

    /** Check if transition curve is playing. */
    /*    bool IsPlaying();*/

    /** @return The transform's location */
    FORCEINLINE const FVector& GetLocation() const { return ViewLocation; }

    /** @return The transform's rotation */
    FORCEINLINE const FVector& GetRotation() const { return ViewRotation; }

    /** @return The look at point for orbiting */
    FORCEINLINE const FVector& GetLookAt() const { return LookAt; }

    /** @return The ortho zoom amount */
    FORCEINLINE float GetOrthoZoom() const { return OrthoZoom; }

    FVector GetForwardVector() const;
    FVector GetRightVector() const;
    FVector GetUpVector() const;

public:
    /** Current viewport Position. */
    FVector ViewLocation;
    /** Current Viewport orientation; valid only for perspective projections. */
    FVector ViewRotation;
    FVector DesiredLocation;
    /** When orbiting, the point we are looking at */
    FVector LookAt;
    /** Viewport start location when animating to another location */
    FVector StartLocation;
    /** Ortho zoom amount */
    float OrthoZoom;
};

class FEditorViewportClient : public FViewportClient
{
public:
    FEditorViewportClient();
    virtual ~FEditorViewportClient() override;

    virtual void Draw(FViewport* Viewport) override;
    virtual UWorld* GetWorld() const override { return nullptr; }
    void Initialize(int32 viewportIndex);
    void Tick(float DeltaTime);
    void Release() const;

    void Input();
    void ResizeViewport(const DXGI_SWAP_CHAIN_DESC& swapchaindesc);
    void ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right);

    bool IsSelected(POINT InPoint) const;

protected:
    /** Camera speed setting */
    int32 CameraSpeedSetting = 1;
    /** Camera speed scalar */
    float CameraSpeedScalar = 1.0f;
    float GridSize;

public:
    FViewport* Viewport;
    int32 ViewportIndex;
    FViewport* GetViewport() const { return Viewport; }
    D3D11_VIEWPORT& GetD3DViewport() const;

public:
    Plane frustumPlanes[6]{};

    //카메라
    /** Viewport camera transform data for perspective viewports */
    FViewportCameraTransform ViewTransformPerspective;
    FViewportCameraTransform ViewTransformOrthographic;
    // 카메라 정보 
    float ViewFOV = 60.0f;
    /** Viewport's stored horizontal field of view (saved in ini files). */
    float FOVAngle = 60.0f;
    float AspectRatio;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    static FVector Pivot;
    static float orthoSize;
    ELevelViewportType ViewportType;
    uint64 ShowFlag;
    EViewModeIndex ViewMode;

    FMatrix View;
    FMatrix Projection;

public: //Camera Movement
    void CameraMoveForward(float InValue);
    void CameraMoveRight(float InValue);
    void CameraMoveUp(float InValue);
    void CameraRotateYaw(float InValue);
    void CameraRotatePitch(float InValue);
    void PivotMoveRight(float InValue);
    void PivotMoveUp(float InValue);

    FMatrix& GetViewMatrix() { return View; }
    FMatrix& GetProjectionMatrix() { return Projection; }
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    bool IsOrtho() const;
    bool IsPerspective() const;
    ELevelViewportType GetViewportType() const;
    void SetViewportType(ELevelViewportType InViewportType);
    void UpdateOrthoCameraLoc();
    EViewModeIndex GetViewMode() const { return ViewMode; }
    void SetViewMode(EViewModeIndex newMode) { ViewMode = newMode; }
    uint64 GetShowFlag() const { return ShowFlag; }
    void SetShowFlag(uint64 newMode) { ShowFlag = newMode; }
    bool GetIsOnRBMouseClick() const { return bRightMouseDown; }

    //Flag Test Code
    static void SetOthoSize(float InValue);

    Plane PlaneFromPoints(const FVector& p0, const FVector& p1, const FVector& p2)
    {
        FVector normal = (p1 - p0).Cross(p2 - p0);
        normal.Normalize();
        float d = -normal.Dot(p0);
        return Plane(normal.X, normal.Y, normal.Z, d);
    }

    void ExtractFrustumPlanesDirect();

private: // Input
    POINT lastMousePos;
    bool bRightMouseDown = false;

public:
    void LoadConfig(const TMap<FString, FString>& config);
    void SaveConfig(TMap<FString, FString>& config) const;

private:
    TMap<FString, FString> ReadIniFile(const FString& filePath) const;
    void WriteIniFile(const FString& filePath, const TMap<FString, FString>& config) const;

public:
    void SetCameraSpeedSetting(const int32& value) { CameraSpeedSetting = value; }
    int32 GetCameraSpeedSetting() const { return CameraSpeedSetting; }
    void SetGridSize(const float& value) { GridSize = value; }
    float GetGridSize() const { return GridSize; }
    float GetCameraSpeedScalar() const { return CameraSpeedScalar; }
    void SetCameraSpeedScalar(float value);

private:
    template <typename T>
    T GetValueFromConfig(const TMap<FString, FString>& config, const FString& key, T defaultValue)
    {
        if (const FString* Value = config.Find(key))
        {
            std::istringstream iss(**Value);
            T value;
            if (iss >> value)
            {
                return value;
            }
        }
        return defaultValue;
    }

public:
    // Gizmo
    void SetGizmoActor(ATransformGizmo* gizmo) { GizmoActor = gizmo; }
    ATransformGizmo* GetGizmoActor() const { return GizmoActor; }

    void SetPickedGizmoComponent(USceneComponent* component) { PickedGizmoComponent = component; }
    USceneComponent* GetPickedGizmoComponent() const { return PickedGizmoComponent; }

    void SetShowGizmo(bool show) { bShowGizmo = show; }
    bool IsShowGizmo() const { return bShowGizmo; }

private:
    ATransformGizmo* GizmoActor = nullptr;
    USceneComponent* PickedGizmoComponent = nullptr;
    bool bShowGizmo = true;
};
