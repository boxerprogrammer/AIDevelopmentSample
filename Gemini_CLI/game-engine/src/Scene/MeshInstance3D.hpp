#pragma once

#include "Scene/Node3D.hpp"
#include "Graphics/RenderItem.hpp"

namespace Scene
{
    /**
     * @brief 3D メッシュを描画するためのノード（Godot の MeshInstance3D 相当）
     *
     * 【Update と Render の分離】
     * - このノード自身は DirectX 12 のコマンドリストを直接操作しません。
     * - CollectRenderItems() で描画パケット (RenderItem) を生成し、レンダラーへ提出します。
     */
    class MeshInstance3D : public Node3D
    {
    public:
        explicit MeshInstance3D(std::string_view name = "MeshInstance3D");
        ~MeshInstance3D() override = default;

        /**
         * @brief 描画するメッシュデータをセット
         */
        void SetMesh(const Graphics::VertexBuffer* vertex_buffer, const Graphics::IndexBuffer* index_buffer) noexcept
        {
            vertex_buffer_ = vertex_buffer;
            index_buffer_ = index_buffer;
        }

        /**
         * @brief 描画アイテム収集の実装
         */
        void CollectRenderItems(std::vector<Graphics::RenderItem>& out_items) override;

    private:
        const Graphics::VertexBuffer* vertex_buffer_ = nullptr;
        const Graphics::IndexBuffer* index_buffer_ = nullptr;
    };
}
