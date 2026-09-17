#pragma once

#include "Scene/Node3D.hpp"
#include "Graphics/RenderItem.hpp"
#include "Resource/Mesh.hpp"
#include "Resource/Material.hpp"
#include <memory>

namespace Scene
{
    /**
     * @brief 3D メッシュリソースをシーン空間上に配置・描画するためのノード（Godot の MeshInstance3D 相当）
     *
     * 【Update と Render の分離 ＆ Flyweight パターン】
     * - このノード自身は重たい頂点バッファを直接抱えず、std::shared_ptr<Resource::Mesh> を保持します。
     * - 同様に材質データ（Material）も共有リソースとして保持し、VRAM と CPU 負荷を最小化します。
     * - CollectRenderItems() で描画パケット (RenderItem) を生成し、レンダラーへ提出します。
     */
    class MeshInstance3D : public Node3D
    {
    public:
        explicit MeshInstance3D(std::string_view name = "MeshInstance3D");
        ~MeshInstance3D() override = default;

        /**
         * @brief 描画するメッシュリソースをセット（共有所有）
         */
        void SetMesh(std::shared_ptr<Resource::Mesh> mesh) noexcept
        {
            mesh_ = std::move(mesh);
        }

        [[nodiscard]] std::shared_ptr<Resource::Mesh> GetMesh() const noexcept { return mesh_; }

        /**
         * @brief 表面材質（マテリアル）リソースをセット（共有所有）
         */
        void SetMaterial(std::shared_ptr<Resource::Material> material) noexcept
        {
            material_ = std::move(material);
        }

        [[nodiscard]] std::shared_ptr<Resource::Material> GetMaterial() const noexcept { return material_; }

        /**
         * @brief 描画アイテム収集の実装
         */
        void CollectRenderItems(std::vector<Graphics::RenderItem>& out_items) override;

    private:
        std::shared_ptr<Resource::Mesh> mesh_;         // 共有メッシュリソース
        std::shared_ptr<Resource::Material> material_; // 共有マテリアルリソース
    };
}
