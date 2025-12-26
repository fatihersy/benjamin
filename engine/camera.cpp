#include "camera.hpp"

using namespace DirectX;

Camera::Camera(float fov, float aspect_ratio, float near_plane, float far_plane) {
    XMVECTOR eye = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    XMVECTOR at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    view_matrix = XMMatrixLookAtLH(eye, at, up);

    projection_matrix = XMMatrixPerspectiveFovLH(fov, aspect_ratio, near_plane, far_plane);
}

Camera::~Camera() {}

void Camera::update() {
    // For now, we'll just keep the camera static.
    // We'll add movement in a later step.
}
