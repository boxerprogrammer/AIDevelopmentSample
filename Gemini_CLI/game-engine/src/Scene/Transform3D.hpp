#pragma once

#include <DirectXMath.h>

namespace Scene
{
    /**
     * @brief 3D 空間における位置 (Translation)・回転 (Rotation)・拡縮 (Scale) を表すクラス
     *
     * 【計算順序】
     * ローカル変換行列 = Scale × Rotation × Translation
     */
    class Transform3D
    {
    public:
        Transform3D() = default;

        Transform3D(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation_radians, const DirectX::XMFLOAT3& scale)
            : position_(position)
            , rotation_(rotation_radians)
            , scale_(scale)
        {
        }

        /**
         * @brief S × R × T の順序でローカル変換行列を計算して取得
         */
        [[nodiscard]] DirectX::XMMATRIX GetLocalMatrix() const noexcept
        {
            const DirectX::XMMATRIX s = DirectX::XMMatrixScaling(scale_.x, scale_.y, scale_.z);
            const DirectX::XMMATRIX r = DirectX::XMMatrixRotationRollPitchYaw(rotation_.x, rotation_.y, rotation_.z);
            const DirectX::XMMATRIX t = DirectX::XMMatrixTranslation(position_.x, position_.y, position_.z);

            return s * r * t;
        }

        // --- 位置の操作 ---
        [[nodiscard]] DirectX::XMFLOAT3 GetPosition() const noexcept { return position_; }
        void SetPosition(float x, float y, float z) noexcept { position_ = { x, y, z }; }
        void SetPosition(const DirectX::XMFLOAT3& pos) noexcept { position_ = pos; }
        void Translate(float dx, float dy, float dz) noexcept
        {
            position_.x += dx;
            position_.y += dy;
            position_.z += dz;
        }

        // --- 回転の操作（オイラー角・ラジアン） ---
        [[nodiscard]] DirectX::XMFLOAT3 GetRotation() const noexcept { return rotation_; }
        void SetRotation(float pitch_x, float yaw_y, float roll_z) noexcept { rotation_ = { pitch_x, yaw_y, roll_z }; }
        void SetRotation(const DirectX::XMFLOAT3& rot) noexcept { rotation_ = rot; }
        void Rotate(float dpitch, float dyaw, float droll) noexcept
        {
            rotation_.x += dpitch;
            rotation_.y += dyaw;
            rotation_.z += droll;
        }

        // --- スケールの操作 ---
        [[nodiscard]] DirectX::XMFLOAT3 GetScale() const noexcept { return scale_; }
        void SetScale(float x, float y, float z) noexcept { scale_ = { x, y, z }; }
        void SetScale(float uniform_scale) noexcept { scale_ = { uniform_scale, uniform_scale, uniform_scale }; }

        // --- 方向ベクトルの取得 ---
        /**
         * @brief オブジェクトが現在向いている前方ベクトル（回転後のローカル +Z 軸）を取得
         */
        [[nodiscard]] DirectX::XMFLOAT3 GetForward() const noexcept
        {
            const DirectX::XMMATRIX r = DirectX::XMMatrixRotationRollPitchYaw(rotation_.x, rotation_.y, rotation_.z);
            const DirectX::XMVECTOR forward = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), r);
            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result, forward);
            return result;
        }

        /**
         * @brief オブジェクトから見た右方向ベクトル（回転後のローカル +X 軸）を取得
         */
        [[nodiscard]] DirectX::XMFLOAT3 GetRight() const noexcept
        {
            const DirectX::XMMATRIX r = DirectX::XMMatrixRotationRollPitchYaw(rotation_.x, rotation_.y, rotation_.z);
            const DirectX::XMVECTOR right = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), r);
            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result, right);
            return result;
        }

        /**
         * @brief オブジェクトから見た上方向ベクトル（回転後のローカル +Y 軸）を取得
         */
        [[nodiscard]] DirectX::XMFLOAT3 GetUp() const noexcept
        {
            const DirectX::XMMATRIX r = DirectX::XMMatrixRotationRollPitchYaw(rotation_.x, rotation_.y, rotation_.z);
            const DirectX::XMVECTOR up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), r);
            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result, up);
            return result;
        }

    private:
        DirectX::XMFLOAT3 position_{ 0.0f, 0.0f, 0.0f }; // 位置 (X, Y, Z)
        DirectX::XMFLOAT3 rotation_{ 0.0f, 0.0f, 0.0f }; // 回転角（ラジアン: Pitch, Yaw, Roll）
        DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };    // 拡大縮小
    };
}
