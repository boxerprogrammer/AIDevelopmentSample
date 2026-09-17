#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <cstdint>

namespace Graphics
{
    /**
     * @brief 平行光源視点の深度（シャドウマップ）テクスチャと DSV/SRV を管理するクラス
     *
     * 【このクラスの責務】
     * 1. Typeless 深度テクスチャの確保（Default Heap: VRAM 専用）
     * 2. シャドウパスで深度書き込みを行うための DSV (Depth Stencil View) の保持
     * 3. メインパスでピクセルシェーダから深度を参照するための SRV (Shader Resource View) の保持
     * 4. シャドウパス用のビューポートとシザー矩形の保持
     * 5. リソースバリア（DEPTH_WRITE <-> PIXEL_SHADER_RESOURCE）の安全な状態遷移
     */
    class ShadowMap
    {
    public:
        // シャドウマップの解像度（2048 x 2048: 鮮明な影と負荷のバランスが最適）
        static constexpr uint32_t kDefaultResolution = 2048;

        /**
         * @brief シャドウマップを生成するコンストラクタ
         * @param device DirectX 12 デバイス
         * @param resolution シャドウマップの縦横解像度（正方形）
         */
        ShadowMap(ID3D12Device* device, uint32_t resolution = kDefaultResolution);

        ~ShadowMap() = default;

        ShadowMap(const ShadowMap&) = delete;
        ShadowMap& operator=(const ShadowMap&) = delete;
        ShadowMap(ShadowMap&&) noexcept = default;
        ShadowMap& operator=(ShadowMap&&) noexcept = default;

        /**
         * @brief シャドウマップを 1.0f（最奥）でクリアする（シャドウパスの直前に呼ぶ）
         */
        void Clear(ID3D12GraphicsCommandList* command_list) const noexcept;

        /**
         * @brief 深度書き込み状態 (DEPTH_WRITE) へリソースバリアを発行
         */
        void TransitionToDepthWrite(ID3D12GraphicsCommandList* command_list);

        /**
         * @brief ピクセルシェーダ読み取り状態 (PIXEL_SHADER_RESOURCE) へリソースバリアを発行
         */
        void TransitionToShaderResource(ID3D12GraphicsCommandList* command_list);

        // --- ハンドル・設定の取得 ---
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept
        {
            return dsv_heap_->GetCPUDescriptorHandleForHeapStart();
        }

        [[nodiscard]] ID3D12DescriptorHeap* GetSRVDescriptorHeap() const noexcept
        {
            return srv_heap_.Get();
        }

        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuHandle() const noexcept
        {
            if (shared_srv_gpu_handle_.ptr != 0)
            {
                return shared_srv_gpu_handle_;
            }
            return srv_heap_->GetGPUDescriptorHandleForHeapStart();
        }

        /**
         * @brief 外部の共有ディスクリプタヒープの指定スロットに SRV を作成し、GPU ハンドルを記憶
         *        ※ DirectX 12 で同一コマンドリストから複数テクスチャを同時参照するために使用
         */
        void CreateShaderResourceViewInHeap(
            ID3D12Device* device,
            D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
            D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);

        [[nodiscard]] const D3D12_VIEWPORT& GetViewport() const noexcept { return viewport_; }
        [[nodiscard]] const D3D12_RECT& GetScissorRect() const noexcept { return scissor_rect_; }
        [[nodiscard]] uint32_t GetResolution() const noexcept { return resolution_; }
        [[nodiscard]] ID3D12Resource* GetResource() const noexcept { return texture_.Get(); }

    private:
        void CreateTextureAndDSV(ID3D12Device* device);
        void CreateSRV(ID3D12Device* device);

    private:
        uint32_t resolution_ = kDefaultResolution;

        Microsoft::WRL::ComPtr<ID3D12Resource> texture_;       // Typeless な深度テクスチャ本体
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap_; // DSV 用ディスクリプタヒープ
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_heap_; // SRV 用ディスクリプタヒープ（シェーダ可視）
        D3D12_GPU_DESCRIPTOR_HANDLE shared_srv_gpu_handle_{ 0 }; // 外部共有ヒープ内での GPU ハンドル

        D3D12_VIEWPORT viewport_{};
        D3D12_RECT scissor_rect_{};
        D3D12_RESOURCE_STATES current_state_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    };
}
