#include "camera.hpp"

using namespace DirectX;

Camera::Camera(float fov, float aspect_ratio, float near_plane, float far_plane, float initial_z)
    : position(0.0f, 0.0f, initial_z), yaw(XM_PI), pitch(0.0f), mouse_sensitivity(0.002f), first_mouse(true),
      fov(fov), near_plane(near_plane), far_plane(far_plane) {
    projection_matrix = XMMatrixPerspectiveFovLH(fov, aspect_ratio, near_plane, far_plane);
    GetCursorPos(&last_mouse_pos);
    update_view_matrix();
}

void Camera::set_aspect_ratio(float aspect_ratio) {
    projection_matrix = XMMatrixPerspectiveFovLH(fov, aspect_ratio, near_plane, far_plane);
}

Camera::~Camera() {}

XMVECTOR Camera::get_forward() const {
    return XMVector3Normalize(XMVectorSet(
        cosf(pitch) * sinf(yaw),
        sinf(pitch),
        cosf(pitch) * cosf(yaw),
        0.0f
    ));
}

XMVECTOR Camera::get_right() const {
    XMVECTOR forward = get_forward();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    return XMVector3Normalize(XMVector3Cross(up, forward));
}

void Camera::update() {
    float speed = 0.1f;

    XMVECTOR pos = XMLoadFloat3(&position);
    XMVECTOR forward = get_forward();
    XMVECTOR right = get_right();

    if (GetAsyncKeyState('W') & 0x8000) pos = XMVectorAdd(pos, XMVectorScale(forward, speed));
    if (GetAsyncKeyState('S') & 0x8000) pos = XMVectorSubtract(pos, XMVectorScale(forward, speed));
    if (GetAsyncKeyState('A') & 0x8000) pos = XMVectorSubtract(pos, XMVectorScale(right, speed));
    if (GetAsyncKeyState('D') & 0x8000) pos = XMVectorAdd(pos, XMVectorScale(right, speed));
    if (GetAsyncKeyState('Q') & 0x8000) pos = XMVectorAdd(pos, XMVectorSet(0.0f, speed, 0.0f, 0.0f));
    if (GetAsyncKeyState('E') & 0x8000) pos = XMVectorSubtract(pos, XMVectorSet(0.0f, speed, 0.0f, 0.0f));

    XMStoreFloat3(&position, pos);

    POINT current_mouse;
    GetCursorPos(&current_mouse);

    if (first_mouse) {
        last_mouse_pos = current_mouse;
        first_mouse = false;
    }

    float dx = static_cast<float>(current_mouse.x - last_mouse_pos.x);
    float dy = static_cast<float>(current_mouse.y - last_mouse_pos.y);
    last_mouse_pos = current_mouse;

    yaw += dx * mouse_sensitivity;
    pitch -= dy * mouse_sensitivity;

    const float max_pitch = XM_PIDIV2 - 0.01f;
    if (pitch > max_pitch) pitch = max_pitch;
    if (pitch < -max_pitch) pitch = -max_pitch;

    update_view_matrix();
}

void Camera::update_view_matrix() {
    XMVECTOR eye = XMLoadFloat3(&position);
    XMVECTOR forward = get_forward();
    XMVECTOR at = XMVectorAdd(eye, forward);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    view_matrix = XMMatrixLookAtLH(eye, at, up);
}
