#include "Graphics/ShadowMap.hpp"
#include <iostream>

namespace Graphics
{
    ShadowMap::ShadowMap(ID3D12Device* device, uint32_t resolution)
        : resolution_(resolution)
        , current_state_(D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        // ビューポートとシザー矩形の設定（シャドウマップの正方形解像度に合わせる）
        viewport_ = {
            0.0f, 0.0f,
            static_cast<float>(resolution_),
            static_cast<float>(resolution_),
            0.0f, 1.0f
        };
        scissor_rect_ = {
            0, 0,
            static_cast<LONG>(resolution_),
            static_cast<LONG>(resolution_)
        };

        CreateTextureAndDSV(device);
        CreateSRV(device);

        std::cout << "[Graphics] ShadowMap (" << resolution_ << "x" << resolution_ << ") を作成しました。\n";
    }

    void ShadowMap::CreateTextureAndDSV(ID3D12Device* device)
    {
        // 1. シャドウテクスチャ本体の作成
        //    【なぜ DXGI_FORMAT_R32_TYPELESS にするのか？】
        //    DirectX 12 では、同一リソースを「書き込み時は深度 (D32_FLOAT)」「読み取り時はカラー (R32_FLOAT)」
        //    として解釈する場合、リソースそのものは型なし (Typeless) で確保する必要があります。
        D3D12_RESOURCE_DESC tex_desc{};
        tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tex_desc.Width = resolution_;
        tex_desc.Height = resolution_;
        tex_desc.DepthOrArraySize = 1;
        tex_desc.MipLevels = 1;
        tex_desc.Format = DXGI_FORMAT_R32_TYPELESS; // 型なしフォーマット
        tex_desc.SampleDesc.Count = 1;
        tex_desc.SampleDesc.Quality = 0;
        tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES heap_props{};
        heap_props.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM 専用メモリ

        // 高速クリアのための最適化クリア値
        D3D12_CLEAR_VALUE clear_val{};
        clear_val.Format = DXGI_FORMAT_D32_FLOAT;
        clear_val.DepthStencil.Depth = 1.0f; // 最奥 (1.0)
        clear_val.DepthStencil.Stencil = 0;

        ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_props,
                D3D12_HEAP_FLAG_NONE,
                &tex_desc,
                current_state_, // 初期状態は DEPTH_WRITE
                &clear_val,
                IID_PPV_ARGS(&texture_)
            ),
            "ShadowMap テクスチャリソースの作成に失敗しました。"
        );

        // 2. DSV ディスクリプタヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc{};
        dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsv_heap_desc.NumDescriptors = 1;
        dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(
            device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap_)),
            "ShadowMap 用 DSV ヒープの作成に失敗しました。"
        );

        // 3. DSV (深度ステンシルビュー) の作成
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
        dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Texture2D.MipSlice = 0;
        dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(texture_.Get(), &dsv_desc, dsv_heap_->GetCPUDescriptorHandleForHeapStart());
    }

    void ShadowMap::CreateSRV(ID3D12Device* device)
    {
        // 1. SRV ディスクリプタヒープの作成（シェーダから参照可能）
        D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc{};
        srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_heap_desc.NumDescriptors = 1;
        srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダ可視

        ThrowIfFailed(
            device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&srv_heap_)),
            "ShadowMap 用 SRV ヒープの作成に失敗しました。"
        );

        // 2. SRV (シェーダリソースビュー) の作成（R32_FLOAT として参照）
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

        device->CreateShaderResourceView(
            texture_.Get(),
            &srv_desc,
            srv_heap_->GetCPUDescriptorHandleForHeapStart()
        );
    }

    void ShadowMap::CreateShaderResourceViewInHeap(
        ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

        device->CreateShaderResourceView(texture_.Get(), &srv_desc, cpu_handle);
        shared_srv_gpu_handle_ = gpu_handle;
    }

    void ShadowMap::Clear(ID3D12GraphicsCommandList* command_list) const noexcept
    {
        // 深度を 1.0f（最奥）でクリア
        command_list->ClearDepthStencilView(GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void ShadowMap::TransitionToDepthWrite(ID3D12GraphicsCommandList* command_list)
    {
        if (current_state_ != D3D12_RESOURCE_STATE_DEPTH_WRITE)
        {
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = texture_.Get();
            barrier.Transition.StateBefore = current_state_;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            command_list->ResourceBarrier(1, &barrier);
            current_state_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
    }

    void ShadowMap::TransitionToShaderResource(ID3D12GraphicsCommandList* command_list)
    {
        if (current_state_ != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        {
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = texture_.Get();
            barrier.Transition.StateBefore = current_state_;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            command_list->ResourceBarrier(1, &barrier);
            current_state_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
    }
}
