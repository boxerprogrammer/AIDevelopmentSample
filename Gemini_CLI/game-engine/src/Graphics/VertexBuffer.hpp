#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <DirectXMath.h>
#include <span>
#include <vector>

namespace Graphics
{
    /**
     * @brief 頂点1つ分のデータ構造（HLSL の VSInput と完全一致させる）
     */
    struct Vertex
    {
        DirectX::XMFLOAT3 position; // 3次元座標 (X, Y, Z)
        DirectX::XMFLOAT4 color;    // 色 (R, G, B, A)
    };

    /**
     * @brief 頂点データを GPU メモリに転送・管理するクラス
     *
     * 【このクラスの責務】
     * 1. 頂点配列を保持する GPU バッファリソースの生成
     * 2. CPU から GPU のメモリ（Upload Heap）への頂点データコピー
     * 3. 描画命令時に GPU に渡す「頂点バッファビュー (D3D12_VERTEX_BUFFER_VIEW)」の提供
     */
    class VertexBuffer
    {
    public:
        /**
         * @brief 頂点配列を受け取り、GPU メモリ上にバッファを作成するコンストラクタ
         * @param device グラフィックスデバイス
         * @param vertices 頂点データのリスト（std::span により std::vector も C言語配列も安全に渡せる）
         */
        VertexBuffer(ID3D12Device* device, std::span<const Vertex> vertices);

        ~VertexBuffer() = default;

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&&) noexcept = default;
        VertexBuffer& operator=(VertexBuffer&&) noexcept = default;

        // 描画時にコマンドリストにセットするビューを取得
        [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& GetView() const noexcept { return buffer_view_; }

        // 頂点数を取得
        [[nodiscard]] uint32_t GetVertexCount() const noexcept { return vertex_count_; }

    private:
        ComPtr<ID3D12Resource> buffer_resource_; // GPU メモリ上のバッファ本体
        D3D12_VERTEX_BUFFER_VIEW buffer_view_{}; // GPU に対するバッファの説明情報（アドレス・サイズ・歩幅）
        uint32_t vertex_count_ = 0;             // 頂点数
    };
}
