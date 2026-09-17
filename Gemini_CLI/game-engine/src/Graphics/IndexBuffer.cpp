#include "Graphics/IndexBuffer.hpp"
#include <cstring>
#include <iostream>

namespace Graphics
{
    IndexBuffer::IndexBuffer(ID3D12Device* device, std::span<const uint16_t> indices)
        : index_count_(static_cast<uint32_t>(indices.size()))
    {
        if (indices.empty())
        {
            throw std::invalid_argument("インデックスデータが空です。");
        }

        const UINT buffer_size = static_cast<UINT>(sizeof(uint16_t) * indices.size());

        // 1. Upload Heap の設定
        D3D12_HEAP_PROPERTIES heap_props{};
        heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC res_desc{};
        res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        res_desc.Alignment = 0;
        res_desc.Width = buffer_size;
        res_desc.Height = 1;
        res_desc.DepthOrArraySize = 1;
        res_desc.MipLevels = 1;
        res_desc.Format = DXGI_FORMAT_UNKNOWN;
        res_desc.SampleDesc.Count = 1;
        res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // 2. GPU 上にメモリ確保
        ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_props,
                D3D12_HEAP_FLAG_NONE,
                &res_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&buffer_resource_)
            ),
            "インデックスバッファの作成に失敗しました。"
        );

        // 3. データのコピー (Map / memcpy / Unmap)
        void* mapped_data = nullptr;
        D3D12_RANGE read_range{ 0, 0 };
        ThrowIfFailed(buffer_resource_->Map(0, &read_range, &mapped_data), "インデックスバッファの Map に失敗しました。");
        std::memcpy(mapped_data, indices.data(), buffer_size);
        buffer_resource_->Unmap(0, nullptr);

        // 4. インデックスバッファビューの作成
        buffer_view_.BufferLocation = buffer_resource_->GetGPUVirtualAddress();
        buffer_view_.SizeInBytes = buffer_size;
        buffer_view_.Format = DXGI_FORMAT_R16_UINT; // 16bit 符号なし整数 (0 ~ 65535)

        std::cout << "[Graphics] インデックスバッファを作成しました。(インデックス数: " << index_count_ 
                  << ", 合計: " << buffer_size << " bytes)\n";
    }
}
