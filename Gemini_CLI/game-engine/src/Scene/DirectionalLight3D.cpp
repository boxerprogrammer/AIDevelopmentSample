#include "Scene/DirectionalLight3D.hpp"
#include <cmath>

namespace Scene
{
    DirectionalLight3D::DirectionalLight3D(std::string_view name)
        : Node3D(name)
    {
        // 初期状態で斜め上から照らす向きに回転を設定
        SetRotation(DirectX::XMConvertToRadians(45.0f), DirectX::XMConvertToRadians(-30.0f), 0.0f);
    }

    DirectX::XMFLOAT3 DirectionalLight3D::GetDirection() const noexcept
    {
        // ノードの回転行列から前方ベクトル (0, 0, 1) を変換して光の方向とする
        const DirectX::XMMATRIX world = GetWorldMatrix();
        const DirectX::XMVECTOR forward = DirectX::XMVector3TransformNormal(
            DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
            world
        );

        DirectX::XMFLOAT3 dir{};
        DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(forward));
        return dir;
    }

    void DirectionalLight3D::SetDirection(float dx, float dy, float dz) noexcept
    {
        direction_ = { dx, dy, dz };
        // 向きベクトルからピッチとヨーを計算してトランスフォームに反映
        const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length > 1e-5f)
        {
            const float norm_y = dy / length;
            const float pitch = std::asin(-norm_y);
            const float yaw = std::atan2(dx, dz);
            SetRotation(pitch, yaw, 0.0f);
        }
    }

    DirectX::XMMATRIX DirectionalLight3D::GetLightViewProjectionMatrix() const noexcept
    {
        const DirectX::XMFLOAT3 dir = GetDirection();
        DirectX::XMVECTOR light_dir = DirectX::XMLoadFloat3(&dir);

        // 太陽光の注視点（原点付近）
        DirectX::XMVECTOR target = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        // 光源の位置（注視点から照射方向と逆向きに一定距離離れた高所）
        constexpr float light_distance = 25.0f;
        DirectX::XMVECTOR light_pos = DirectX::XMVectorSubtract(target, DirectX::XMVectorScale(light_dir, light_distance));

        // 上方向ベクトルの決定（真上・真下付近の特異点を考慮）
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        if (std::abs(dir.y) > 0.99f)
        {
            up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        }

        // 光源視点のビュー行列
        const DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(light_pos, target, up);

        // 平行光源用の直交投影プロジェクション行列
        const DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicLH(
            shadow_box_size_,
            shadow_box_size_,
            0.1f,
            60.0f
        );

        return view * proj;
    }
}
