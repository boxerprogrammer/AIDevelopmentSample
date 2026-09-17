#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <cstring>
#include <concepts>

namespace Graphics
{
    /**
     * @brief 定数バッファ (Constant Buffer) を管理する RAII テンプレートクラス
     *
     * 【このクラスの責務】
     * 1. DirectX 12 の仕様である「256 バイト境界アライメント」を満たす GPU メモリの確保
     * 2. 永続マップ (Persistent Mapping) による、オーバーヘッドのない安全な CPU -> GPU データ転送
     * 3. コマンドリストに直接渡せる GPU 仮想アドレス (D3D12_GPU_VIRTUAL_ADDRESS) の提供
     *
     * @tparam T シェーダに送る C++ の構造体型
     */
    template <typename T>
    class ConstantBuffer
    {
    public:
        /**
         * @brief 定数バッファを作成し、GPU メモリを CPU にマップする
         * @param device グラフィックスデバイス
         */
        explicit ConstantBuffer(ID3D12Device* device)
        {
            // DirectX 12 のルール: 定数バッファのサイズは必ず「256 バイトの倍数」でなければならない
            aligned_size_ = (sizeof(T) + 255) & ~255;

            // 1. CPU から書き込める Upload Heap を設定
            D3D12_HEAP_PROPERTIES heap_props{};
            heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
            heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            // 2. バッファリソースの仕様
            D3D12_RESOURCE_DESC res_desc{};
            res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            res_desc.Alignment = 0;
            res_desc.Width = aligned_size_;
            res_desc.Height = 1;
            res_desc.DepthOrArraySize = 1;
            res_desc.MipLevels = 1;
            res_desc.Format = DXGI_FORMAT_UNKNOWN;
            res_desc.SampleDesc.Count = 1;
            res_desc.SampleDesc.Quality = 0;
            res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            // 3. GPU 上にメモリ確保
            ThrowIfFailed(
                device->CreateCommittedResource(
                    &heap_props,
                    D3D12_HEAP_FLAG_NONE,
                    &res_desc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&buffer_resource_)
                ),
                "定数バッファの作成に失敗しました。"
            );

            // 4. 永続マップ (Persistent Mapping)
            // DirectX 12 では、Upload Heap のメモリはアプリ起動中に一度 Map しておけば、
            // 毎フレーム Unmap しなくても安全に memcpy して構わない仕様になっています。
            D3D12_RANGE read_range{ 0, 0 }; // CPU からは読まない
            ThrowIfFailed(
                buffer_resource_->Map(0, &read_range, &mapped_data_),
                "定数バッファの Map に失敗しました。"
            );
        }

        /**
         * @brief デストラクタ: 破棄時に安全に Unmap を行う
         */
        ~ConstantBuffer()
        {
            if (buffer_resource_ && mapped_data_)
            {
                buffer_resource_->Unmap(0, nullptr);
                mapped_data_ = nullptr;
            }
        }

        ConstantBuffer(const ConstantBuffer&) = delete;
        ConstantBuffer& operator=(const ConstantBuffer&) = delete;

        ConstantBuffer(ConstantBuffer&& other) noexcept
            : buffer_resource_(std::move(other.buffer_resource_))
            , mapped_data_(other.mapped_data_)
            , aligned_size_(other.aligned_size_)
        {
            other.mapped_data_ = nullptr;
            other.aligned_size_ = 0;
        }

        ConstantBuffer& operator=(ConstantBuffer&& other) noexcept
        {
            if (this != &other)
            {
                if (buffer_resource_ && mapped_data_)
                {
                    buffer_resource_->Unmap(0, nullptr);
                }
                buffer_resource_ = std::move(other.buffer_resource_);
                mapped_data_ = other.mapped_data_;
                aligned_size_ = other.aligned_size_;
                other.mapped_data_ = nullptr;
                other.aligned_size_ = 0;
            }
            return *this;
        }

        /**
         * @brief CPU 側のデータを GPU メモリへ転送する
         * @param data 転送するデータ
         */
        void Update(const T& data)
        {
            if (mapped_data_)
            {
                std::memcpy(mapped_data_, &data, sizeof(T));
            }
        }

        /**
         * @brief コマンドリストに渡す GPU 仮想アドレスを取得
         */
        [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept
        {
            return buffer_resource_->GetGPUVirtualAddress();
        }

        [[nodiscard]] size_t GetAlignedSize() const noexcept { return aligned_size_; }

    private:
        ComPtr<ID3D12Resource> buffer_resource_; // GPU メモリ上のバッファ
        void* mapped_data_ = nullptr;            // CPU 側から書き込めるメモリポインタ
        size_t aligned_size_ = 0;                // 256バイト境界に切り上げた実サイズ
    };
}
