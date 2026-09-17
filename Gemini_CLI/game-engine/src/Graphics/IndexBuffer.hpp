#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <span>
#include <cstdint>

namespace Graphics
{
    /**
     * @brief 頂点インデックスを GPU メモリに保持するクラス
     *
     * 【このクラスの責務】
     * 1. 頂点の接続順（ポリゴンを構成する頂点番号のリスト）を GPU に転送
     * 2. 重複した頂点の再定義を省き、頂点バッファのメモリ消費を抑える
     * 3. コマンドリストの IASetIndexBuffer に渡す D3D12_INDEX_BUFFER_VIEW の提供
     */
    class IndexBuffer
    {
    public:
        /**
         * @brief インデックスバッファを生成するコンストラクタ
         * @param device グラフィックスデバイス
         * @param indices 16bit 符号なし整数 (uint16_t) のインデックス配列
         */
        IndexBuffer(ID3D12Device* device, std::span<const uint16_t> indices);

        ~IndexBuffer() = default;

        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;
        IndexBuffer(IndexBuffer&&) noexcept = default;
        IndexBuffer& operator=(IndexBuffer&&) noexcept = default;

        [[nodiscard]] const D3D12_INDEX_BUFFER_VIEW& GetView() const noexcept { return buffer_view_; }
        [[nodiscard]] uint32_t GetIndexCount() const noexcept { return index_count_; }

    private:
        ComPtr<ID3D12Resource> buffer_resource_;
        D3D12_INDEX_BUFFER_VIEW buffer_view_{};
        uint32_t index_count_ = 0;
    };
}
