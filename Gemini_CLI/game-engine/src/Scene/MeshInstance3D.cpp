#include "Scene/MeshInstance3D.hpp"

namespace Scene
{
    MeshInstance3D::MeshInstance3D(std::string_view name)
        : Node3D(name)
    {
    }

    void MeshInstance3D::CollectRenderItems(std::vector<Graphics::RenderItem>& out_items)
    {
        // 共有メッシュリソースが設定されており、バッファが有効であれば描画パケットを生成
        if (mesh_ && mesh_->GetVertexBuffer() && mesh_->GetIndexBuffer())
        {
            Graphics::RenderItem item{};
            // 親子階層を考慮した最新のワールド変換行列を格納
            DirectX::XMStoreFloat4x4(&item.world_matrix, GetWorldMatrix());
            item.vertex_buffer = mesh_->GetVertexBuffer();
            item.index_buffer = mesh_->GetIndexBuffer();
            item.material = material_.get();

            out_items.push_back(item);
        }

        // 子ノードの描画アイテムも再帰的に収集
        Node::CollectRenderItems(out_items);
    }
}
