#pragma once

#include <DirectXMath.h>

class Camera {
public:
    Camera(float fov, float aspect_ratio, float near_plane, float far_plane);
    ~Camera();

    void update();

    DirectX::XMMATRIX get_view_matrix() const { return view_matrix; }
    DirectX::XMMATRIX get_projection_matrix() const { return projection_matrix; }

private:
    DirectX::XMMATRIX view_matrix;
    DirectX::XMMATRIX projection_matrix;
};
