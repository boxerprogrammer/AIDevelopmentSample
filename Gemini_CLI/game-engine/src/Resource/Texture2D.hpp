#pragma once

#include "Resource/Resource.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace Resource
{
    /**
     * @brief 2D テクスチャアセットをカプセル化するリソースクラス（Godot の Texture2D 相当）
     *
     * 【このクラスの責務】
     * 1. GPU 専用メモリ (Default Heap) 上のテクスチャリソースの排他的所有
     * 2. シェーダから参照するための SRV (Shader Resource View) ディスクリプタヒープの保持
     * 3. 画像ファイル読み込み (PNG/JPG 等) およびプロシージャル生成ファクトリの提供
     * 4. ResourceLoader による Flyweight パターンでの自動共有
     */
    class Texture2D : public Resource
    {
    public:
        Texture2D(ID3D12Device* device,
                  ID3D12CommandQueue* command_queue,
                  const uint8_t* pixels,
                  uint32_t width,
                  uint32_t height,
                  std::string_view path = "memory://texture");

        ~Texture2D() override = default;

        // コピー禁止（GPU リソースの多重解放防止）
        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;

        // ムーブ許可
        Texture2D(Texture2D&&) noexcept = default;
        Texture2D& operator=(Texture2D&&) noexcept = default;

        // --- ゲッター ---
        [[nodiscard]] ID3D12Resource* GetResource() const noexcept { return texture_resource_.Get(); }
        [[nodiscard]] ID3D12DescriptorHeap* GetDescriptorHeap() const noexcept { return srv_heap_.Get(); }
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle() const noexcept
        {
            if (shared_srv_gpu_handle_.ptr != 0)
            {
                return shared_srv_gpu_handle_;
            }
            return srv_heap_->GetGPUDescriptorHandleForHeapStart();
        }

        /**
         * @brief 外部の共有ディスクリプタヒープの指定スロットに SRV を作成し、GPU ハンドルを記憶
         *        ※ DirectX 12 で単一コマンドリストから複数テクスチャ（シャドウマップ＋アルベド等）を参照するために使用
         */
        void CreateShaderResourceViewInHeap(
            ID3D12Device* device,
            D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
            D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);

        [[nodiscard]] uint32_t GetWidth() const noexcept { return width_; }
        [[nodiscard]] uint32_t GetHeight() const noexcept { return height_; }

        // --- ファクトリ関数群 ---

        /**
         * @brief 生ピクセル配列 (RGBA8) からテクスチャを生成
         */
        static std::shared_ptr<Texture2D> CreateFromPixels(
            ID3D12Device* device,
            ID3D12CommandQueue* command_queue,
            const uint8_t* pixels,
            uint32_t width,
            uint32_t height,
            std::string_view path = "memory://texture");

        /**
         * @brief 画像ファイル (PNG, JPG, BMP 等) からテクスチャを読み込み生成
         */
        static std::shared_ptr<Texture2D> CreateFromFile(
            ID3D12Device* device,
            ID3D12CommandQueue* command_queue,
            std::string_view file_path);

        /**
         * @brief デフォルトの 1x1 白テクスチャを生成（テクスチャ未設定マテリアル用）
         */
        static std::shared_ptr<Texture2D> CreateWhite(
            ID3D12Device* device,
            ID3D12CommandQueue* command_queue);

        /**
         * @brief デバッグ・地面用のチェッカーボード（市松模様）テクスチャを生成
         */
        static std::shared_ptr<Texture2D> CreateCheckerboard(
            ID3D12Device* device,
            ID3D12CommandQueue* command_queue,
            uint32_t width = 256,
            uint32_t height = 256,
            uint32_t check_size = 32);

    private:
        /**
         * @brief CPU メモリ上のピクセルデータを GPU (Default Heap) にアップロード
         */
        void UploadTextureData(
            ID3D12Device* device,
            ID3D12CommandQueue* command_queue,
            const uint8_t* pixels,
            uint32_t width,
            uint32_t height);

        /**
         * @brief シェーダ参照用の SRV ディスクリプタヒープとビューを作成
         */
        void CreateShaderResourceView(ID3D12Device* device);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> texture_resource_;       // GPU 専用メモリ上の 2D テクスチャ本体
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_heap_;         // 個別 SRV ディスクリプタヒープ
        D3D12_GPU_DESCRIPTOR_HANDLE shared_srv_gpu_handle_{ 0 };        // 外部共有ヒープ内での GPU ハンドル
        uint32_t width_ = 0;
        uint32_t height_ = 0;
    };
}
