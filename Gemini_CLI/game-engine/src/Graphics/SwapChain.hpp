#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <cstdint>
#include <array>

namespace Graphics
{
    class GraphicsDevice;

    /**
     * @brief スワップチェインと画面表示用バックバッファを管理するクラス
     *
     * 【このクラスの責務】
     * 1. ダブルバッファリング（表画面・裏画面の切り替え）の管理
     * 2. レンダーターゲットビュー (RTV) 用ディスクリプタヒープの作成
     * 3. バックバッファに対応する RTV の作成とハンドル提供
     * 4. 垂直同期 (VSync) に合わせた画面表示 (Present)
     */
    class SwapChain
    {
    public:
        // 教育用として理解しやすい「ダブルバッファリング（表・裏の2枚）」を採用
        static constexpr uint32_t kBufferCount = 2;

        /**
         * @brief スワップチェインと RTV を生成するコンストラクタ
         * @param device グラフィックスデバイス
         * @param hwnd ウィンドウハンドル
         * @param width 描画領域の幅
         * @param height 描画領域の高さ
         */
        SwapChain(const GraphicsDevice& device, HWND hwnd, uint32_t width, uint32_t height);

        ~SwapChain() = default;

        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain(SwapChain&&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        /**
         * @brief バックバッファを画面に転送（フリップ）して表示する
         * @param sync_interval 垂直同期の待機（1: 60fps固定等のVSync有効, 0: 制限なし）
         */
        void Present(uint32_t sync_interval = 1);

        /**
         * @brief 現在描画対象となっているバックバッファのインデックス (0 または 1) を取得
         */
        [[nodiscard]] uint32_t GetCurrentBackBufferIndex() const noexcept;

        /**
         * @brief 現在のバックバッファリソース（テクスチャ本体）を取得
         */
        [[nodiscard]] ID3D12Resource* GetCurrentRenderTarget() const noexcept;

        /**
         * @brief 現在のバックバッファのレンダーターゲットビュー（CPU ディスクリプタハンドル）を取得
         */
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const noexcept;

        [[nodiscard]] uint32_t GetWidth() const noexcept { return width_; }
        [[nodiscard]] uint32_t GetHeight() const noexcept { return height_; }

    private:
        void CreateSwapChain(const GraphicsDevice& device, HWND hwnd);
        void CreateRtvDescriptorHeap(const GraphicsDevice& device);
        void CreateRenderTargets(const GraphicsDevice& device);

    private:
        ComPtr<IDXGISwapChain4> swap_chain_;                                // スワップチェイン本体
        ComPtr<ID3D12DescriptorHeap> rtv_heap_;                             // RTV ディスクリプタヒープ（名札置き場）
        std::array<ComPtr<ID3D12Resource>, kBufferCount> render_targets_;  // バックバッファ（描画キャンバス本体）
        uint32_t rtv_descriptor_size_ = 0;                                  // RTV 1つ分のバイトサイズ（ヒープ内の歩幅）
        uint32_t width_ = 0;
        uint32_t height_ = 0;
    };
}
