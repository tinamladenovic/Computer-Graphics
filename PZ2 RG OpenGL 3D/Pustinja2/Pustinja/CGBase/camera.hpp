#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();

    void Move(float dx, float dy, float dt);

    void Rotate(float dx, float dy, float dt);

    glm::vec3 GetPosition();
    glm::vec3 GetTarget();
    glm::vec3 GetUp();

    // Dodajte set funkcije za poziciju, target i up
    void SetPosition(const glm::vec3& position);
    void SetTarget(const glm::vec3& target);
    void SetUp(const glm::vec3& up);
    // Dodajte funkcije za zumiranje
    void ZoomIn();
    void ZoomOut();

    void SetBirdPerspective(float fov, float aspectRatio, float nearClip, float farClip);

    float GetZoomFactor() const {
        return mZoomFactor;
    }

    void SetYaw(float yaw) {
        mYaw = yaw;
        updateVectors();
    }

    void SetPitch(float pitch) {
        mPitch = pitch;
        updateVectors();
    }

    void SetPerspectiveProjection(float fov, float aspect, float near, float far) {
        mProjectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);
    }

    void SetOrthographicProjection(float left, float right, float bottom, float top, float near, float far) {
        mProjectionMatrix = glm::ortho(left, right, bottom, top, near, far);
    }


private:
    glm::vec3 mWorldUp;
    glm::vec3 mPosition;
    glm::vec3 mFront;
    glm::vec3 mUp;
    glm::vec3 mRight;
    glm::vec3 mTarget;
    glm::vec3 mVelocity;

    float mMoveSpeed;
    float mLookSpeed;
    float mPitch;
    float mYaw;
    float mPlayerHeight;


    void updateVectors();
    float mZoomFactor;

    glm::mat4 mProjectionMatrix;
    glm::mat4 mViewMatrix;
    void updateViewProjection();

   
};

