#include "CameraComponent.h"
#include "Math/JungleMath.h"
#include "UnrealEd/EditorViewportClient.h"
#include "LevelEditor/SLevelEditor.h"


void UCameraComponent::InitializeComponent()
{
	Super::InitializeComponent();
	RelativeLocation = FVector(0.0f, 0.0f, 0.5f);
	FOV = 60.f;
}

void UCameraComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

	Input();
	// QuatRotation = JungleMath::EulerToQuaternion(RelativeRotation);
}

void UCameraComponent::Input()
{
    return;
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) // VK_RBUTTON은 마우스 오른쪽 버튼을 나타냄
	{
		if (!bRightMouseDown)
		{
			// 마우스 오른쪽 버튼을 처음 눌렀을 때, 마우스 위치 초기화
			bRightMouseDown = true;
			GetCursorPos(&lastMousePos);
		}
		else
		{
			// 마우스 이동량 계산
			POINT currentMousePos;
			GetCursorPos(&currentMousePos);

			// 마우스 이동 차이 계산
			int32 deltaX = currentMousePos.x - lastMousePos.x;
			int32 deltaY = currentMousePos.y - lastMousePos.y;

			// Yaw(좌우 회전) 및 Pitch(상하 회전) 값 변경
			RotateYaw(deltaX * 0.1f);  // X 이동에 따라 좌우 회전
			RotatePitch(deltaY * 0.1f);  // Y 이동에 따라 상하 회전

			// 새로운 마우스 위치 저장
			lastMousePos = currentMousePos;
		}
		if (GetAsyncKeyState('A') & 0x8000)
		{
			MoveRight(-1.f);
		}
		if (GetAsyncKeyState('D') & 0x8000)
		{
			MoveRight(1.f);
		}
		if (GetAsyncKeyState('W') & 0x8000)
		{
			MoveForward(1.f);
		}
		if (GetAsyncKeyState('S') & 0x8000)
		{
			MoveForward(-1.f);
		}
		if (GetAsyncKeyState('E') & 0x8000)
		{
			MoveUp(1.f);
		}
		if (GetAsyncKeyState('Q') & 0x8000)
		{
			MoveUp(-1.f);
		}
	}
	else
	{
		bRightMouseDown = false; // 마우스 오른쪽 버튼을 떼면 상태 초기화
	}

}

void UCameraComponent::MoveForward(float _Value)
{
	RelativeLocation = RelativeLocation + GetForwardVector() * GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar() * _Value;
}

void UCameraComponent::MoveRight(float _Value)
{
	//FVector newRight = FVector(GetRightVector().X, GetRightVector().Y, 0.0f);
	RelativeLocation = RelativeLocation + GetRightVector() * GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar() * _Value;
}

void UCameraComponent::MoveUp(float _Value)
{
	RelativeLocation.Z += _Value * GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
}

void UCameraComponent::RotateYaw(float _Value)
{
    RelativeRotation.Yaw += _Value * GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
	// RelativeRotation.Z += _Value * GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
}

void UCameraComponent::RotatePitch(float _Value)
{
    RelativeRotation.Pitch += FMath::Clamp((RelativeRotation.Pitch + _Value), -90.f, 90.f);
    
	// RelativeRotation.Y = FMath::Clamp(RelativeRotation.Y + _Value, -90.f, 90.f);
}
