#include "Scene/Camera3D.hpp"

namespace Scene
{
    Camera3D::Camera3D(std::string_view name)
        : Node3D(name)
    {
    }

    DirectX::XMMATRIX Camera3D::GetViewMatrix() const noexcept
    {
        // カメラのワールド行列: 「カメラが世界の中でどこにいて、どの方向を向いているか」
        const DirectX::XMMATRIX camera_world = GetWorldMatrix();

        // View 行列: 「世界中の物体を、カメラの目線（原点）へと引き戻す変換」
        // 数学的に、View 行列はカメラのワールド変換行列の「逆行列 (Inverse)」となります
        return DirectX::XMMatrixInverse(nullptr, camera_world);
    }

    DirectX::XMMATRIX Camera3D::GetProjectionMatrix() const noexcept
    {
        // 遠近法（透視投影）プロジェクション行列の計算
        return DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XMConvertToRadians(fov_degrees_),
            aspect_ratio_,
            near_z_,
            far_z_
        );
    }
}
