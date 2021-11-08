#include "Camera.h"

Camera::Camera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& forwardDirection,
               const DirectX::XMFLOAT3& rightDirection, float focalDist, float fov) :
    mPitch(0.0f), mYaw(0.0f) {
    
    mPosition = DirectX::XMLoadFloat3(&position);
    DirectX::XMVectorSetW(mPosition, 1.0f);

    mForwardDirection = DirectX::XMLoadFloat3(&forwardDirection);
    mForwardDirection = DirectX::XMVector3Normalize(mForwardDirection);
    mRightDirection = DirectX::XMLoadFloat3(&rightDirection);

    mUpDirection = DirectX::XMVector3Cross(mForwardDirection, mRightDirection);

    mCameraCB.focalDist = focalDist;
    mCameraCB.position = position;
    mCameraCB.fov = fov;

    DirectX::XMStoreFloat3(&mCameraCB.direction, mForwardDirection);
    DirectX::XMStoreFloat3(&mCameraCB.right, mRightDirection);
    DirectX::XMStoreFloat3(&mCameraCB.up, mUpDirection);

    mCameraDirection = mForwardDirection;
    mCameraRight = mRightDirection;
}

void Camera::WalkForward(float frameTime) {
    mPosition = DirectX::XMVectorMultiplyAdd(mCameraDirection, DirectX::XMVectorSet(frameTime, frameTime, frameTime, 1.0f), mPosition);
}

void Camera::WalkBackward(float frameTime) {
    WalkForward(frameTime * -1.0f);
}

void Camera::StrafeRight(float frameTime) {
    mPosition = DirectX::XMVectorMultiplyAdd(mCameraRight, DirectX::XMVectorSet(frameTime, frameTime, frameTime, 1.0f), mPosition);
}

void Camera::StrafeLeft(float frameTime) {
    StrafeRight(frameTime * -1.0f);
}

void Camera::RotateX(float frameTime, float movement) {
    mPitch += movement * frameTime;
}

void Camera::RotateY(float frameTime, float movement) {
    mYaw += movement * frameTime;
}

void Camera::Lift(float frameTime) {
    mPosition = DirectX::XMVectorMultiplyAdd(mUpDirection, DirectX::XMVectorSet(frameTime, frameTime, frameTime, 1.0f), mPosition);
}

void Camera::Lower(float frameTime) {
    Lift(frameTime * -1.0f);
}

const DirectX::XMFLOAT3& Camera::GetPosition() const {
    return mCameraCB.position;
}

const Camera::CameraCB& Camera::GetCameraCB() const {
    return mCameraCB;
}

float Camera::GetCameraFov() const {
    return mCameraCB.fov;
}

void Camera::SetCameraFov(float fov) {
    mCameraCB.fov = fov;
}

void Camera::Construct() {

    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);
    mCameraDirection = DirectX::XMVector3TransformCoord(mForwardDirection, rotationMatrix);
    mCameraRight = DirectX::XMVector3TransformCoord(mRightDirection, rotationMatrix);
    mUpDirection = DirectX::XMVector3Cross(mCameraDirection, mCameraRight);

    DirectX::XMStoreFloat3(&mCameraCB.direction, mCameraDirection);
    DirectX::XMStoreFloat3(&mCameraCB.right, mCameraRight);
    DirectX::XMStoreFloat3(&mCameraCB.up, mUpDirection);
    DirectX::XMStoreFloat3(&mCameraCB.position, mPosition);
}


