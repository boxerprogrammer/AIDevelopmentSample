#pragma once

#include "Resource/Resource.hpp"
#include "Graphics/VertexBuffer.hpp"
#include "Graphics/IndexBuffer.hpp"
#include <memory>
#include <span>
#include <d3d12.h>

namespace Resource
{
    /**
     * @brief 3D 幾何形状（頂点配列とインデックス配列）を保持するリソースクラス
     *
     * 【このクラスの責務】
     * 1. 頂点バッファ (VertexBuffer) とインデックスバッファ (IndexBuffer) の安全な保持
     * 2. 基本的なプリミティブ形状（キューブ、平面など）の生成ファクトリ
     * 3. 複数の MeshInstance3D ノード間でのメモリ共有（Flyweight パターン）
     */
    class Mesh : public Resource
    {
    public:
        /**
         * @brief 既存のバッファからメッシュを生成
         */
        Mesh(std::unique_ptr<Graphics::VertexBuffer> vertex_buffer,
             std::unique_ptr<Graphics::IndexBuffer> index_buffer,
             std::string_view path = "");

        ~Mesh() override = default;

        // ゲッター関数群
        [[nodiscard]] const Graphics::VertexBuffer* GetVertexBuffer() const noexcept { return vertex_buffer_.get(); }
        [[nodiscard]] const Graphics::IndexBuffer* GetIndexBuffer() const noexcept { return index_buffer_.get(); }

        // --- プリミティブ形状のファクトリメソッド ---

        /**
         * @brief 各面に固有色を持つサイコロ型立方体メッシュを生成
         * @param device DirectX 12 デバイス
         * @param size 立方体の 1 辺の長さ
         */
        static std::shared_ptr<Mesh> CreateCube(ID3D12Device* device, float size = 1.0f);

        /**
         * @brief XZ 平面に広がる 2 トライアングルの平面メッシュを生成
         * @param device DirectX 12 デバイス
         * @param width X 方向の幅
         * @param depth Z 方向の奥行き
         */
        static std::shared_ptr<Mesh> CreatePlane(ID3D12Device* device, float width = 10.0f, float depth = 10.0f);

    private:
        std::unique_ptr<Graphics::VertexBuffer> vertex_buffer_; // 頂点バッファ
        std::unique_ptr<Graphics::IndexBuffer> index_buffer_;   // インデックスバッファ
    };
}
