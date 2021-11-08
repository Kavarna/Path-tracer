#pragma once



#include <Oblivion.h>


OBLIVION_ALIGN(16)
class Camera {
public:
    struct CameraCB {
        DirectX::XMFLOAT3 position;
        float focalDist;
        DirectX::XMFLOAT3 direction;
        float fov;
        DirectX::XMFLOAT3 right;
        float pad1;
        DirectX::XMFLOAT3 up;
        float pad2;

    };

public:

    Camera() = default;
    Camera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& forwardDirection,
           const DirectX::XMFLOAT3& rightDirection, float focalDist, float fov);

public:
    void WalkForward(float frameTime);
    void WalkBackward(float frameTime);
    void StrafeRight(float frameTime);
    void StrafeLeft(float frameTime);
    void RotateX(float frameTime, float movement);
    void RotateY(float frameTime, float movement);
    void Lift(float frameTime);
    void Lower(float frameTime);

    const DirectX::XMFLOAT3& GetPosition() const;

    const CameraCB& GetCameraCB() const;

    float GetCameraFov() const;
    void SetCameraFov(float fov);

    void Construct();

private:
    DirectX::XMVECTOR mPosition;

    DirectX::XMVECTOR mForwardDirection;
    DirectX::XMVECTOR mRightDirection;
    DirectX::XMVECTOR mUpDirection;

    DirectX::XMVECTOR mCameraDirection;
    DirectX::XMVECTOR mCameraRight;
    DirectX::XMVECTOR mCameraUp;

    float mPitch, mYaw;

    // Pass this to the shaders
    CameraCB mCameraCB;
};


