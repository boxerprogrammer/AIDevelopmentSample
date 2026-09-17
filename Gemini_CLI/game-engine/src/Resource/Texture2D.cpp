#include "Resource/Texture2D.hpp"
#include "Graphics/DirectXHelper.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <vector>
#include <stdexcept>

namespace Resource
{
    Texture2D::Texture2D(
        ID3D12Device* device,
        ID3D12CommandQueue* command_queue,
        const uint8_t* pixels,
        uint32_t width,
        uint32_t height,
        std::string_view path)
        : Resource(path)
        , width_(width)
        , height_(height)
    {
        UploadTextureData(device, command_queue, pixels, width, height);
        CreateShaderResourceView(device);

        std::cout << "[Resource] Texture2D を生成しました: " << GetPath()
                  << " (" << width_ << "x" << height_ << ")\n";
    }

    void Texture2D::UploadTextureData(
        ID3D12Device* device,
        ID3D12CommandQueue* command_queue,
        const uint8_t* pixels,
        uint32_t width,
        uint32_t height)
    {
        // 1. テクスチャ本体（Default Heap: GPU 専用メモリ）の定義と作成
        D3D12_RESOURCE_DESC tex_desc{};
        tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tex_desc.Width = width;
        tex_desc.Height = height;
        tex_desc.DepthOrArraySize = 1;
        tex_desc.MipLevels = 1;
        tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.SampleDesc.Quality = 0;
        tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES default_heap_props{};
        default_heap_props.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM 専用メモリ

        Graphics::ThrowIfFailed(
            device->CreateCommittedResource(
                &default_heap_props,
                D3D12_HEAP_FLAG_NONE,
                &tex_desc,
                D3D12_RESOURCE_STATE_COPY_DEST, // まずは GPU コピーの転送先として作成
                nullptr,
                IID_PPV_ARGS(&texture_resource_)
            ),
            "Default Heap テクスチャリソースの作成に失敗しました。"
        );

        // 2. 中間アップロードバッファ（Upload Heap）の正確なサイズとレイアウトを計算
        //    ※ DirectX 12 では、各行のピッチ（横幅バイト数）が 256 バイトアライメントを満たす必要があります
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT num_rows = 0;
        UINT64 row_size_in_bytes = 0;
        UINT64 total_bytes = 0;
        device->GetCopyableFootprints(&tex_desc, 0, 1, 0, &footprint, &num_rows, &row_size_in_bytes, &total_bytes);

        D3D12_RESOURCE_DESC upload_desc{};
        upload_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        upload_desc.Width = total_bytes;
        upload_desc.Height = 1;
        upload_desc.DepthOrArraySize = 1;
        upload_desc.MipLevels = 1;
        upload_desc.Format = DXGI_FORMAT_UNKNOWN;
        upload_desc.SampleDesc.Count = 1;
        upload_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES upload_heap_props{};
        upload_heap_props.Type = D3D12_HEAP_TYPE_UPLOAD; // CPU 書き込み可能メモリ

        Microsoft::WRL::ComPtr<ID3D12Resource> upload_buffer;
        Graphics::ThrowIfFailed(
            device->CreateCommittedResource(
                &upload_heap_props,
                D3D12_HEAP_FLAG_NONE,
                &upload_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&upload_buffer)
            ),
            "中間アップロードバッファの作成に失敗しました。"
        );

        // 3. CPU 上のピクセルデータを 256 バイトパディングを考慮しながら中間バッファへコピー
        void* mapped_ptr = nullptr;
        Graphics::ThrowIfFailed(
            upload_buffer->Map(0, nullptr, &mapped_ptr),
            "中間アップロードバッファの Map に失敗しました。"
        );

        auto* dest_bytes = static_cast<uint8_t*>(mapped_ptr);
        const uint32_t src_pitch = width * 4; // RGBA 8bit = 4 bytes per pixel

        for (uint32_t y = 0; y < height; ++y)
        {
            memcpy(
                dest_bytes + footprint.Offset + y * footprint.Footprint.RowPitch,
                pixels + y * src_pitch,
                src_pitch
            );
        }

        upload_buffer->Unmap(0, nullptr);

        // 4. 一回限りの GPU コマンドリストを作成し、中間バッファから Default Heap へテクスチャデータを転送
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_allocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd_list;

        Graphics::ThrowIfFailed(
            device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_allocator)),
            "テクスチャ転送用コマンドアロケータの作成に失敗しました。"
        );
        Graphics::ThrowIfFailed(
            device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_allocator.Get(), nullptr, IID_PPV_ARGS(&cmd_list)),
            "テクスチャ転送用コマンドリストの作成に失敗しました。"
        );

        // コピー元（中間バッファ）とコピー先（テクスチャ本体）の指定
        D3D12_TEXTURE_COPY_LOCATION dst_loc{};
        dst_loc.pResource = texture_resource_.Get();
        dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_loc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src_loc{};
        src_loc.pResource = upload_buffer.Get();
        src_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src_loc.PlacedFootprint = footprint;

        cmd_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);

        // リソースバリア: コピー先状態 (COPY_DEST) から シェーダ読み取り状態 (PIXEL_SHADER_RESOURCE) へ遷移
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture_resource_.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd_list->ResourceBarrier(1, &barrier);

        Graphics::ThrowIfFailed(cmd_list->Close(), "テクスチャ転送コマンドリストの Close に失敗しました。");

        // 5. コマンドキューで実行し、GPU コピー完了を Fence で同期（待機）
        ID3D12CommandList* cmd_lists[] = { cmd_list.Get() };
        command_queue->ExecuteCommandLists(1, cmd_lists);

        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        Graphics::ThrowIfFailed(
            device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
            "テクスチャ転送用 Fence の作成に失敗しました。"
        );

        Graphics::ThrowIfFailed(command_queue->Signal(fence.Get(), 1), "Fence Signal に失敗しました。");

        HANDLE fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fence->GetCompletedValue() < 1)
        {
            fence->SetEventOnCompletion(1, fence_event);
            WaitForSingleObject(fence_event, INFINITE);
        }
        CloseHandle(fence_event);

        // ※ 関数を抜けると upload_buffer は自動破棄され、GPU 専用メモリ (Default Heap) だけが保持されます
    }

    void Texture2D::CreateShaderResourceView(ID3D12Device* device)
    {
        // シェーダから参照するための SRV ディスクリプタヒープを作成（1 スロット）
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダから参照可能
        heap_desc.NodeMask = 0;

        Graphics::ThrowIfFailed(
            device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&srv_heap_)),
            "Texture2D 用 SRV ディスクリプタヒープの作成に失敗しました。"
        );

        // SRV（シェーダリソースビュー）の生成
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

        device->CreateShaderResourceView(
            texture_resource_.Get(),
            &srv_desc,
            srv_heap_->GetCPUDescriptorHandleForHeapStart()
        );
    }

    void Texture2D::CreateShaderResourceViewInHeap(
        ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

        device->CreateShaderResourceView(texture_resource_.Get(), &srv_desc, cpu_handle);
        shared_srv_gpu_handle_ = gpu_handle;
    }

    std::shared_ptr<Texture2D> Texture2D::CreateFromPixels(
        ID3D12Device* device,
        ID3D12CommandQueue* command_queue,
        const uint8_t* pixels,
        uint32_t width,
        uint32_t height,
        std::string_view path)
    {
        return std::make_shared<Texture2D>(device, command_queue, pixels, width, height, path);
    }

    std::shared_ptr<Texture2D> Texture2D::CreateFromFile(
        ID3D12Device* device,
        ID3D12CommandQueue* command_queue,
        std::string_view file_path)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        // stb_image を使用して画像ファイルを RGBA 8bit 形式 (4 チャンネル) でデコード
        unsigned char* data = stbi_load(file_path.data(), &width, &height, &channels, 4);
        if (!data)
        {
            std::cerr << "[Texture2D] 画像ファイルの読み込みに失敗しました: " << file_path
                      << " (理由: " << stbi_failure_reason() << ")\n";
            std::cerr << "[Texture2D] 代替としてチェッカーボードテクスチャを生成します。\n";
            return CreateCheckerboard(device, command_queue);
        }

        auto texture = CreateFromPixels(
            device,
            command_queue,
            data,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            file_path
        );

        stbi_image_free(data);
        return texture;
    }

    std::shared_ptr<Texture2D> Texture2D::CreateWhite(
        ID3D12Device* device,
        ID3D12CommandQueue* command_queue)
    {
        // 1x1 の純白ピクセル (0xFFFFFFFF = RGBA(255, 255, 255, 255))
        constexpr uint32_t white_pixel = 0xFFFFFFFF;
        return CreateFromPixels(
            device,
            command_queue,
            reinterpret_cast<const uint8_t*>(&white_pixel),
            1, 1,
            "builtin://Texture/White"
        );
    }

    std::shared_ptr<Texture2D> Texture2D::CreateCheckerboard(
        ID3D12Device* device,
        ID3D12CommandQueue* command_queue,
        uint32_t width,
        uint32_t height,
        uint32_t check_size)
    {
        std::vector<uint32_t> pixels(width * height);

        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                // 市松模様の白マス・濃灰色マスの判定
                const bool is_light = ((x / check_size) % 2) == ((y / check_size) % 2);
                // RGBA 形式: 明るいグレー (0xFFE8E8E8) と 濃いスレートグレー (0xFF4A4A5A)
                pixels[y * width + x] = is_light ? 0xFFEEEEEE : 0xFF4A4A5A;
            }
        }

        return CreateFromPixels(
            device,
            command_queue,
            reinterpret_cast<const uint8_t*>(pixels.data()),
            width, height,
            "builtin://Texture/Checkerboard"
        );
    }
}
