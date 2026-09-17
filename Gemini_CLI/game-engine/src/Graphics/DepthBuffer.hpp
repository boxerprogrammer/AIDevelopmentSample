#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <cstdint>

namespace Graphics
{
    /**
     * @brief 深度バッファ（Z バッファ）と DSV (Depth Stencil View) を管理するクラス
     *
     * 【このクラスの責務】
     * 1. 画面と同じ解像度の深度テクスチャリソースの確保 (Default Heap)
     * 2. 深度ステンシルビュー (DSV) 用ディスクリプタヒープの作成
     * 3. 毎フレームの深度バッファクリア (ClearDepthStencilView) の提供
     * 4. コマンドリストへの DSV CPU ハンドル提供
     */
    class DepthBuffer
    {
    public:
        // 深度値の標準フォーマット（32bit 浮動小数点数: 0.0=最も手前, 1.0=最も奥）
        static constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D32_FLOAT;

        /**
         * @brief 深度バッファを生成するコンストラクタ
         * @param device グラフィックスデバイス
         * @param width 画面の幅（ピクセル）
         * @param height 画面の高さ（ピクセル）
         */
        DepthBuffer(ID3D12Device* device, uint32_t width, uint32_t height);

        ~DepthBuffer() = default;

        DepthBuffer(const DepthBuffer&) = delete;
        DepthBuffer& operator=(const DepthBuffer&) = delete;
        DepthBuffer(DepthBuffer&&) noexcept = default;
        DepthBuffer& operator=(DepthBuffer&&) noexcept = default;

        /**
         * @brief 深度バッファをクリアする（毎フレームの描画開始前に呼び出す）
         * @param command_list コマンドリスト
         * @param depth_clear_value クリアする深度値（通常は最も遠い 1.0f）
         */
        void Clear(ID3D12GraphicsCommandList* command_list, float depth_clear_value = 1.0f) const noexcept;

        // DSV の CPU ディスクリプタハンドルを取得
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept
        {
            return dsv_heap_->GetCPUDescriptorHandleForHeapStart();
        }

        [[nodiscard]] ID3D12Resource* GetResource() const noexcept { return depth_texture_.Get(); }

    private:
        ComPtr<ID3D12Resource> depth_texture_; // GPU 上の深度テクスチャ本体
        ComPtr<ID3D12DescriptorHeap> dsv_heap_; // DSV 用ディスクリプタヒープ（名札置き場）
        uint32_t width_ = 0;
        uint32_t height_ = 0;
    };
}
