#include "Graphics/VertexBuffer.hpp"
#include <cstring>
#include <iostream>

namespace Graphics
{
    VertexBuffer::VertexBuffer(ID3D12Device* device, std::span<const Vertex> vertices)
        : vertex_count_(static_cast<uint32_t>(vertices.size()))
    {
        if (vertices.empty())
        {
            throw std::invalid_argument("頂点データが空です。");
        }

        const UINT buffer_size = static_cast<UINT>(sizeof(Vertex) * vertices.size());

        // 1. ヒーププロパティの設定
        // 【アップロードヒープ (D3D12_HEAP_TYPE_UPLOAD)】:
        // CPU から直接書き込め、GPU から読み取れる共有メモリ領域。
        // 初学者向けに、面倒な転送用中間バッファやコピーコマンドを使わずに1ステップでデータをGPUに送れます。
        D3D12_HEAP_PROPERTIES heap_props{};
        heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        // 2. リソースの仕様（バッファの形状）
        D3D12_RESOURCE_DESC res_desc{};
        res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // 1次元の平坦なバイト配列
        res_desc.Alignment = 0;
        res_desc.Width = buffer_size;
        res_desc.Height = 1;
        res_desc.DepthOrArraySize = 1;
        res_desc.MipLevels = 1;
        res_desc.Format = DXGI_FORMAT_UNKNOWN;
        res_desc.SampleDesc.Count = 1;
        res_desc.SampleDesc.Quality = 0;
        res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // 3. GPU 上にメモリを確保
        ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_props,
                D3D12_HEAP_FLAG_NONE,
                &res_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ, // アップロードヒープは最初から読み取り可能状態で作成する規則
                nullptr,
                IID_PPV_ARGS(&buffer_resource_)
            ),
            "頂点バッファの作成に失敗しました。"
        );

        // 4. CPU から GPU のメモリへ頂点配列をコピー (Map / memcpy / Unmap)
        void* mapped_data = nullptr;
        D3D12_RANGE read_range{ 0, 0 }; // CPU からはこのメモリを「読まない」ことを GPU に明示（最適化）
        ThrowIfFailed(buffer_resource_->Map(0, &read_range, &mapped_data), "頂点バッファの Map に失敗しました。");
        std::memcpy(mapped_data, vertices.data(), buffer_size);
        buffer_resource_->Unmap(0, nullptr);

        // 5. 頂点バッファビュー（GPU が描画時に参照する目次）を作成
        buffer_view_.BufferLocation = buffer_resource_->GetGPUVirtualAddress(); // GPU メモリ上の番地
        buffer_view_.StrideInBytes = sizeof(Vertex);                           // 頂点1個あたりのサイズ（歩幅）
        buffer_view_.SizeInBytes = buffer_size;                               // バッファ全体のサイズ

        std::cout << "[Graphics] 頂点バッファを作成しました。(頂点数: " << vertex_count_ 
                  << ", 合計: " << buffer_size << " bytes)\n";
    }
}
