#include "camera.hpp"

Camera::Camera() {
    mPosition = glm::vec3(0.0f, 0.0f, 10.0f);

    mWorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    mRight = glm::vec3(1.0f, 0.0f, 0.0f);
    mVelocity = glm::vec3(0.0f);
    mPitch = 0.0f;
    mYaw = -90.0f;
    mMoveSpeed = 8.0f;
    mLookSpeed = 64.0f;
    mPlayerHeight = 2.0f;
    mZoomFactor = 1.0f;
    updateVectors();
    ZoomIn();
    ZoomOut();
    void SetBirdPerspective(float fov, float aspectRatio, float nearClip, float farClip);
}

void
Camera::Move(float dx, float dy, float dt) {
    mPosition += (dx * mRight + dy * mFront) * mMoveSpeed * dt;
    updateVectors();
}

void
Camera::Rotate(float dx, float dy, float dt) {
    float RotateVelocity = mLookSpeed * dt;
    mYaw += dx * RotateVelocity;
    mPitch += dy * RotateVelocity;

    if (mPitch > 89.0f) {
        mPitch = 89.0f;
    }
    if (mPitch < -89.0f) {
        mPitch = -89.0f;
    }

    updateVectors();
}


glm::vec3
Camera::GetPosition() {
    return mPosition;
}

glm::vec3
Camera::GetTarget() {
    return mPosition + mFront;
}

glm::vec3
Camera::GetUp() {
    return mUp;
}

void
Camera::updateVectors() {
    mFront.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    mFront.y = sin(glm::radians(mPitch));
    mFront.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    mFront = glm::normalize(mFront);
    mRight = glm::normalize(glm::cross(mFront, mWorldUp));
    mUp = glm::normalize(glm::cross(mRight, mFront));
    mPosition.y = mPosition.y < mPlayerHeight
        ? mPlayerHeight
        : mPosition.y > mPlayerHeight
        ? mPlayerHeight
        : mPosition.y;
}
void Camera::SetPosition(const glm::vec3& position) {
    mPosition = position;
    updateVectors();
    updateViewProjection();
}

void Camera::SetTarget(const glm::vec3& target) {
    mTarget = target;
    updateVectors();
    updateViewProjection();
}

void Camera::SetUp(const glm::vec3& up) {
    mUp = up;
    updateVectors();
    updateViewProjection();
}

void Camera::ZoomIn() {
    // Implementacija zumiranja unapred
    mZoomFactor *= 1.1f;
}

void Camera::ZoomOut() {
    // Implementacija zumiranja unazad
    mZoomFactor /= 1.1f;
}

void Camera::SetBirdPerspective(float fov, float aspectRatio, float nearClip, float farClip) {
    mPosition = glm::vec3(0.0f, 20.0f, 0.0f);  // Postavite visinu kamere prema potrebi
    mTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // Gledajte prema centru scene
    mUp = glm::vec3(0.0f, 0.0f, -1.0f);  // Postavite gore prema negativnom z-u

    // Postavljanje projekcione matrice za ptičju perspektivu
    mProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
    mViewMatrix = glm::lookAt(mPosition, mTarget, mUp);
}

void Camera::updateViewProjection() {
    mViewMatrix = glm::lookAt(mPosition, mTarget, mUp);
    // Ako želite ažurirati projekcionu matricu na osnovu promenljivih poput fov, near, far, to uradite ovde.
    // Na primer: mProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
}

