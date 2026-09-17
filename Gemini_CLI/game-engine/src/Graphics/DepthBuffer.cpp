#include "Graphics/DepthBuffer.hpp"
#include <iostream>

namespace Graphics
{
    DepthBuffer::DepthBuffer(ID3D12Device* device, uint32_t width, uint32_t height)
        : width_(width)
        , height_(height)
    {
        // 1. 深度テクスチャリソースの作成
        // 【デフォルトヒープ (D3D12_HEAP_TYPE_DEFAULT)】:
        // CPU から直接触ることはできず、GPU 専用の超高速 VRAM 領域に確保します。
        // 深度バッファは GPU がピクセルを描画するたびに Z 値を激しく読み書きするため、Default Heap が必須です。
        D3D12_HEAP_PROPERTIES heap_props{};
        heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC res_desc{};
        res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元の画面テクスチャ
        res_desc.Alignment = 0;
        res_desc.Width = width_;
        res_desc.Height = height_;
        res_desc.DepthOrArraySize = 1;
        res_desc.MipLevels = 1;
        res_desc.Format = kDepthFormat;                         // 32bit 浮動小数点 (0.0 ~ 1.0)
        res_desc.SampleDesc.Count = 1;
        res_desc.SampleDesc.Quality = 0;
        res_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 深度バッファとして使用する宣言

        // 最適化されたクリア値（ClearDepthStencilView で使う値と一致させることでクリア処理が高速化される）
        D3D12_CLEAR_VALUE clear_value{};
        clear_value.Format = kDepthFormat;
        clear_value.DepthStencil.Depth = 1.0f; // 最も奥 (1.0) でクリア
        clear_value.DepthStencil.Stencil = 0;

        ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_props,
                D3D12_HEAP_FLAG_NONE,
                &res_desc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度書き込み可能状態で作成
                &clear_value,
                IID_PPV_ARGS(&depth_texture_)
            ),
            "深度バッファリソースの作成に失敗しました。"
        );

        // 2. DSV (Depth Stencil View) 用ディスクリプタヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.NumDescriptors = 1;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // 深度ステンシルビュー専用
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(
            device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_heap_)),
            "DSV 用ディスクリプタヒープの作成に失敗しました。"
        );

        // 3. 深度ステンシルビュー (名札) の作成
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
        dsv_desc.Format = kDepthFormat;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(
            depth_texture_.Get(),
            &dsv_desc,
            dsv_heap_->GetCPUDescriptorHandleForHeapStart()
        );

        std::cout << "[Graphics] 深度バッファ (Zバッファ: " << width_ << "x" << height_ << ") を作成しました。\n";
    }

    void DepthBuffer::Clear(ID3D12GraphicsCommandList* command_list, float depth_clear_value) const noexcept
    {
        // 画面全体の Z 値を初期化（通常は 1.0f = 最も遠い状態）
        command_list->ClearDepthStencilView(
            GetDSVHandle(),
            D3D12_CLEAR_FLAG_DEPTH,
            depth_clear_value,
            0,
            0,
            nullptr
        );
    }
}
