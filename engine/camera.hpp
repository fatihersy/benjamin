#pragma once

#include <DirectXMath.h>
#include <windows.h>

class Camera {
public:
    Camera(float fov, float aspect_ratio, float near_plane, float far_plane, float initial_z = 5.0f);
    ~Camera();

    void update();
    void set_aspect_ratio(float aspect_ratio);

    DirectX::XMMATRIX get_view_matrix() const { return view_matrix; }
    DirectX::XMMATRIX get_projection_matrix() const { return projection_matrix; }

private:
    void update_view_matrix();
    DirectX::XMVECTOR get_forward() const;
    DirectX::XMVECTOR get_right() const;

    DirectX::XMMATRIX view_matrix;
    DirectX::XMMATRIX projection_matrix;

    DirectX::XMFLOAT3 position;
    float yaw;
    float pitch;
    float mouse_sensitivity;
    POINT last_mouse_pos;
    bool first_mouse;

    float fov;
    float near_plane;
    float far_plane;
};
