#pragma once

#include "Scene/Node3D.hpp"
#include <DirectXMath.h>

namespace Scene
{
    /**
     * @brief 無限遠から平行に降り注ぐ太陽光を表すライトノード（Godot の DirectionalLight3D 相当）
     *
     * 【このクラスの責務】
     * 1. 光の照射方向 (Light Direction) の管理（ノードの回転姿勢から自動計算）
     * 2. 光の色 (Light Color) と強度 (Intensity) の保持
     * 3. シーン全体を照らす環境光 (Ambient Light) の設定
     */
    class DirectionalLight3D : public Node3D
    {
    public:
        explicit DirectionalLight3D(std::string_view name = "DirectionalLight3D");
        ~DirectionalLight3D() override = default;

        /**
         * @brief ノードの回転姿勢から計算された正規化済み光線方向ベクトルを取得
         *        ※ 光が向かう方向（照射先への向き）を返します
         */
        [[nodiscard]] DirectX::XMFLOAT3 GetDirection() const noexcept;

        // 手動で向きを直接設定することも可能（オイラー角自動変換）
        void SetDirection(float dx, float dy, float dz) noexcept;

        // --- 色・強度パラメータ ---
        void SetColor(float r, float g, float b) noexcept { color_ = { r, g, b }; }
        void SetColor(const DirectX::XMFLOAT3& color) noexcept { color_ = color; }
        [[nodiscard]] const DirectX::XMFLOAT3& GetColor() const noexcept { return color_; }
        [[nodiscard]] DirectX::XMFLOAT3& GetColor() noexcept { return color_; }

        void SetIntensity(float intensity) noexcept { intensity_ = intensity; }
        [[nodiscard]] float GetIntensity() const noexcept { return intensity_; }

        void SetAmbientColor(float r, float g, float b) noexcept { ambient_color_ = { r, g, b }; }
        void SetAmbientColor(const DirectX::XMFLOAT3& color) noexcept { ambient_color_ = color; }
        [[nodiscard]] const DirectX::XMFLOAT3& GetAmbientColor() const noexcept { return ambient_color_; }
        [[nodiscard]] DirectX::XMFLOAT3& GetAmbientColor() noexcept { return ambient_color_; }

        // --- シャドウマッピング用パラメータ・行列 ---
        /**
         * @brief 光源視点の直交投影 View-Projection 行列を計算
         */
        [[nodiscard]] DirectX::XMMATRIX GetLightViewProjectionMatrix() const noexcept;

        void SetShadowBias(float bias) noexcept { shadow_bias_ = bias; }
        [[nodiscard]] float GetShadowBias() const noexcept { return shadow_bias_; }

        void SetShadowStrength(float strength) noexcept { shadow_strength_ = strength; }
        [[nodiscard]] float GetShadowStrength() const noexcept { return shadow_strength_; }

        void SetShadowBoxSize(float size) noexcept { shadow_box_size_ = size; }
        [[nodiscard]] float GetShadowBoxSize() const noexcept { return shadow_box_size_; }

    private:
        DirectX::XMFLOAT3 color_{ 1.0f, 0.98f, 0.9f };          // 太陽光の暖色系ホワイト
        float intensity_ = 1.0f;                                // 光源強度
        DirectX::XMFLOAT3 ambient_color_{ 0.15f, 0.15f, 0.20f }; // 暗部を照らす微弱な環境光
        DirectX::XMFLOAT3 direction_{ 0.5f, -1.0f, 0.5f };      // 照射方向

        float shadow_bias_ = 0.0015f;                           // 自己遮蔽アーティファクト防止バイアス
        float shadow_strength_ = 0.65f;                         // 影の濃さ (0.0=無効, 1.0=完全な闇)
        float shadow_box_size_ = 18.0f;                         // 影を描画する直交投影の範囲
    };
}
