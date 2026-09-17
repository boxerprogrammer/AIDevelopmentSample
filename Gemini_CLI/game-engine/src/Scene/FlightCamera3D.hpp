#pragma once

#include "Scene/Camera3D.hpp"

namespace Scene
{
    /**
     * @brief キーボードとマウスで 3D 空間を自由に移動・旋回できるデバッグ用フライトカメラ
     *
     * 【操作方法】
     * - マウス右ドラッグ: 視線の回転（上下 Pitch / 左右 Yaw）
     * - W / S: 前進 / 後退（カメラの向いている方向）
     * - A / D: 左 / 右 平行移動（ストライフ）
     * - E / Space: 上昇
     * - Q / C: 下降
     * - Left Shift: 移動速度ブースト
     * - マウスホイール: 移動速度の増減
     */
    class FlightCamera3D : public Camera3D
    {
    public:
        explicit FlightCamera3D(std::string_view name = "FlightCamera3D");
        ~FlightCamera3D() override = default;

        /**
         * @brief 毎フレームの更新処理（ユーザー入力を検知してカメラを移動・回転）
         */
        void OnProcess(float delta) override;

        // パラメータ取得・設定
        void SetMoveSpeed(float speed) noexcept { move_speed_ = speed; }
        [[nodiscard]] float GetMoveSpeed() const noexcept { return move_speed_; }

        void SetBoostMultiplier(float multiplier) noexcept { boost_multiplier_ = multiplier; }
        [[nodiscard]] float GetBoostMultiplier() const noexcept { return boost_multiplier_; }

        void SetMouseSensitivity(float sensitivity) noexcept { mouse_sensitivity_ = sensitivity; }
        [[nodiscard]] float GetMouseSensitivity() const noexcept { return mouse_sensitivity_; }

        void SetInputEnabled(bool enabled) noexcept { is_input_enabled_ = enabled; }
        [[nodiscard]] bool IsInputEnabled() const noexcept { return is_input_enabled_; }

        [[nodiscard]] float GetYaw() const noexcept { return yaw_; }
        [[nodiscard]] float GetPitch() const noexcept { return pitch_; }

        /**
         * @brief 視線角度（Yaw / Pitch）を直接設定
         */
        void SetViewAngles(float pitch_radians, float yaw_radians) noexcept;

    private:
        float move_speed_ = 5.0f;               // 通常の移動速度（単位/秒）
        float boost_multiplier_ = 2.5f;         // Shift 押下時の速度倍率
        float mouse_sensitivity_ = 0.003f;      // マウス回転感度（ラジアン/ピクセル）
        bool is_input_enabled_ = true;          // 入力受け付けフラグ

        float yaw_ = 0.0f;                      // 左右回転角（ラジアン、Y軸周り）
        float pitch_ = 0.0f;                    // 上下回転角（ラジアン、X軸周り）
    };
}
