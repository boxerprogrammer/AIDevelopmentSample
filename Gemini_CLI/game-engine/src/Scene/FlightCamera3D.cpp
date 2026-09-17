#include "Scene/FlightCamera3D.hpp"
#include "Core/Input.hpp"

#include <algorithm>
#include <cmath>

namespace Scene
{
    FlightCamera3D::FlightCamera3D(std::string_view name)
        : Camera3D(name)
    {
    }

    void FlightCamera3D::OnProcess(float delta)
    {
        if (!is_input_enabled_)
        {
            return;
        }

        // --- 1. マウスホイールによる移動速度の動的調整 ---
        const float wheel_delta = Core::Input::GetMouseWheelDelta();
        if (std::abs(wheel_delta) > 1e-4f)
        {
            move_speed_ = std::clamp(move_speed_ + wheel_delta * 1.0f, 0.5f, 50.0f);
        }

        // --- 2. マウス右ドラッグによる視線回転 (Pitch & Yaw) ---
        if (Core::Input::IsMouseButtonHeld(Core::MouseButton::Right))
        {
            const DirectX::XMFLOAT2 mouse_delta = Core::Input::GetMouseDelta();

            // 水平移動で Yaw (左右首振り、Y軸回転)、垂直移動で Pitch (上下首振り、X軸回転)
            yaw_ += mouse_delta.x * mouse_sensitivity_;
            pitch_ += mouse_delta.y * mouse_sensitivity_;

            // 上下の首振り角度を -89度 〜 +89度 に制限（カメラが反転するジンバルロックの防止）
            constexpr float max_pitch = DirectX::XMConvertToRadians(89.0f);
            pitch_ = std::clamp(pitch_, -max_pitch, max_pitch);

            // 左右回転角を 0 〜 2π に収める
            constexpr float two_pi = DirectX::XM_2PI;
            while (yaw_ > two_pi) yaw_ -= two_pi;
            while (yaw_ < 0.0f) yaw_ += two_pi;

            // 回転をトランスフォームに反映（ロール角 Z は水平を保つため 0 に固定）
            SetRotation(pitch_, yaw_, 0.0f);
        }

        // --- 3. キーボードによる移動処理 ---
        float current_speed = move_speed_;
        if (Core::Input::IsKeyHeld(Core::KeyCode::Shift))
        {
            current_speed *= boost_multiplier_; // Shift 押下でブースト
        }

        DirectX::XMVECTOR move_dir = DirectX::XMVectorZero();

        // W / S: 前進 / 後退（カメラの現在の向き基準）
        if (Core::Input::IsKeyHeld(Core::KeyCode::W))
        {
            const auto fwd = GetForward();
            move_dir = DirectX::XMVectorAdd(move_dir, DirectX::XMLoadFloat3(&fwd));
        }
        if (Core::Input::IsKeyHeld(Core::KeyCode::S))
        {
            const auto fwd = GetForward();
            move_dir = DirectX::XMVectorSubtract(move_dir, DirectX::XMLoadFloat3(&fwd));
        }

        // A / D: 左 / 右 平行移動（カメラの横方向基準）
        if (Core::Input::IsKeyHeld(Core::KeyCode::D))
        {
            const auto right = GetRight();
            move_dir = DirectX::XMVectorAdd(move_dir, DirectX::XMLoadFloat3(&right));
        }
        if (Core::Input::IsKeyHeld(Core::KeyCode::A))
        {
            const auto right = GetRight();
            move_dir = DirectX::XMVectorSubtract(move_dir, DirectX::XMLoadFloat3(&right));
        }

        // E / Space: 上昇（ワールド Y 軸方向）
        if (Core::Input::IsKeyHeld(Core::KeyCode::E) || Core::Input::IsKeyHeld(Core::KeyCode::Space))
        {
            move_dir = DirectX::XMVectorAdd(move_dir, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        }

        // Q / C: 下降（ワールド Y 軸方向）
        if (Core::Input::IsKeyHeld(Core::KeyCode::Q) || Core::Input::IsKeyHeld(Core::KeyCode::C))
        {
            move_dir = DirectX::XMVectorSubtract(move_dir, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        }

        // 移動入力がある場合、ベクトルを正規化して移動を適用
        if (!DirectX::XMVector3Equal(move_dir, DirectX::XMVectorZero()))
        {
            move_dir = DirectX::XMVector3Normalize(move_dir);
            const DirectX::XMVECTOR delta_move = DirectX::XMVectorScale(move_dir, current_speed * delta);

            DirectX::XMFLOAT3 dm{};
            DirectX::XMStoreFloat3(&dm, delta_move);
            Translate(dm.x, dm.y, dm.z);
        }
    }

    void FlightCamera3D::SetViewAngles(float pitch_radians, float yaw_radians) noexcept
    {
        constexpr float max_pitch = DirectX::XMConvertToRadians(89.0f);
        pitch_ = std::clamp(pitch_radians, -max_pitch, max_pitch);
        yaw_ = yaw_radians;
        SetRotation(pitch_, yaw_, 0.0f);
    }
}
