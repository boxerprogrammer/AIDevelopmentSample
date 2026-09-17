#pragma once

#include <DirectXMath.h>

namespace Resource
{
    class Material;
}

namespace Graphics
{
    class VertexBuffer;
    class IndexBuffer;

    /**
     * @brief シーンツリーからレンダラーへ渡される描画コマンド情報（抽出データ）
     *
     * 【Update と Render の責務分離】
     * - Node が直接 DirectX 12 の CommandList を触るのではなく、
     *   描画に必要な純粋なデータ（行列、バッファ、マテリアル）だけをこの構造体に詰めて提出します。
     */
    struct RenderItem
    {
        DirectX::XMFLOAT4X4 world_matrix;            // ワールド変換行列
        const VertexBuffer* vertex_buffer = nullptr; // 頂点バッファ
        const IndexBuffer* index_buffer = nullptr;   // インデックスバッファ
        const Resource::Material* material = nullptr; // 表面材質（マテリアル）
    };
}
