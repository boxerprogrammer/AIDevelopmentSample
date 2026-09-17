#include "Scene/MeshInstance3D.hpp"

namespace Scene
{
    MeshInstance3D::MeshInstance3D(std::string_view name)
        : Node3D(name)
    {
    }

    void MeshInstance3D::CollectRenderItems(std::vector<Graphics::RenderItem>& out_items)
    {
        // 有効な頂点バッファとインデックスバッファが設定されていれば描画パケットを生成
        if (vertex_buffer_ && index_buffer_)
        {
            Graphics::RenderItem item{};
            // 親子階層を考慮した最新のワールド変換行列を格納
            DirectX::XMStoreFloat4x4(&item.world_matrix, GetWorldMatrix());
            item.vertex_buffer = vertex_buffer_;
            item.index_buffer = index_buffer_;

            out_items.push_back(item);
        }

        // 子ノードの描画アイテムも再帰的に収集
        Node::CollectRenderItems(out_items);
    }
}
