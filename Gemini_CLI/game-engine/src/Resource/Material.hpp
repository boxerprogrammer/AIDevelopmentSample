#pragma once

#include "Resource/Resource.hpp"
#include <DirectXMath.h>
#include <memory>
#include <string_view>

namespace Resource
{
    class Texture2D;

    /**
     * @brief 3D オブジェクトの表面材質（色、光沢度、反射率など）を定義するリソースクラス
     *
     * 【このクラスの責務】
     * 1. 表面の色 (Albedo Color) の保持
     * 2. 鏡面反射ハイライトの鋭さ (Specular Power / Shininess) と強さ (Specular Intensity) の制御
     * 3. 表面テクスチャ (Texture2D) の保持
     * 4. 複数の MeshInstance3D ノード間での材質共有（Flyweight パターン）
     */
    class Material : public Resource
    {
    public:
        explicit Material(std::string_view path = "");
        ~Material() override = default;

        // --- 色の設定・取得 ---
        void SetColor(float r, float g, float b, float a = 1.0f) noexcept { color_ = { r, g, b, a }; }
        void SetColor(const DirectX::XMFLOAT4& color) noexcept { color_ = color; }
        [[nodiscard]] const DirectX::XMFLOAT4& GetColor() const noexcept { return color_; }
        [[nodiscard]] DirectX::XMFLOAT4& GetColor() noexcept { return color_; }

        // --- 鏡面反射（ツヤ）の設定・取得 ---
        void SetSpecularPower(float power) noexcept { specular_power_ = power; }
        [[nodiscard]] float GetSpecularPower() const noexcept { return specular_power_; }

        void SetSpecularIntensity(float intensity) noexcept { specular_intensity_ = intensity; }
        [[nodiscard]] float GetSpecularIntensity() const noexcept { return specular_intensity_; }

        // --- テクスチャの設定・取得 ---
        void SetTexture(std::shared_ptr<Texture2D> texture) noexcept { texture_ = std::move(texture); }
        [[nodiscard]] std::shared_ptr<Texture2D> GetTexture() const noexcept { return texture_; }

        // --- プリセット生成ファクトリ ---
        /**
         * @brief 標準的な光沢を持つ白いデフォルトマテリアル
         */
        static std::shared_ptr<Material> CreateDefault();

        /**
         * @brief 金属やプラスチックのような強いツヤを持つマテリアル
         */
        static std::shared_ptr<Material> CreateShiny(const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

        /**
         * @brief 粘土や布のような光沢の鈍いマットマテリアル
         */
        static std::shared_ptr<Material> CreateMatte(const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    private:
        DirectX::XMFLOAT4 color_{ 1.0f, 1.0f, 1.0f, 1.0f }; // 表面の反射色 (R, G, B, A)
        float specular_power_ = 32.0f;                       // ハイライトの鋭さ（値が大きいほど鋭い点になる）
        float specular_intensity_ = 0.5f;                    // ハイライトの明るさ（0.0 で無光沢、1.0 で最大）
        std::shared_ptr<Texture2D> texture_;                 // 表面テクスチャ（nullptr の場合はデフォルト白テクスチャを使用）
    };
}
