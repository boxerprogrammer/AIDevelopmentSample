#pragma once

#include "Scene/Node3D.hpp"

namespace Scene
{
    /**
     * @brief 3D 空間の視点を表すカメラノード（Godot の Camera3D 相当）
     *
     * 【このクラスの責務】
     * 1. 3D 空間上の位置・向きに基づく View 行列の生成
     * 2. 視野角 (FOV) やアスペクト比に基づく Projection 行列の生成
     * 3. カメラ自身の移動・回転（Node3D の機能）がそのまま視点の動きに連動
     */
    class Camera3D : public Node3D
    {
    public:
        explicit Camera3D(std::string_view name = "Camera3D");
        ~Camera3D() override = default;

        /**
         * @brief カメラの現在の位置と姿勢から View 行列（カメラ基準の座標変換）を計算
         */
        [[nodiscard]] DirectX::XMMATRIX GetViewMatrix() const noexcept;

        /**
         * @brief 遠近法（透視投影）を表す Projection 行列を計算
         */
        [[nodiscard]] DirectX::XMMATRIX GetProjectionMatrix() const noexcept;

        // パラメータ設定
        void SetFov(float fov_degrees) noexcept { fov_degrees_ = fov_degrees; }
        [[nodiscard]] float GetFov() const noexcept { return fov_degrees_; }

        void SetAspectRatio(float aspect_ratio) noexcept { aspect_ratio_ = aspect_ratio; }
        [[nodiscard]] float GetAspectRatio() const noexcept { return aspect_ratio_; }

        void SetClipPlanes(float near_z, float far_z) noexcept
        {
            near_z_ = near_z;
            far_z_ = far_z;
        }

    private:
        float fov_degrees_ = 60.0f;           // 垂直視野角（度）
        float aspect_ratio_ = 16.0f / 9.0f;   // アスペクト比（幅 / 高さ）
        float near_z_ = 0.1f;                 // ニアクリップ距離
        float far_z_ = 100.0f;                // ファークリップ距離
    };
}
