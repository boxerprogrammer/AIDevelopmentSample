#include "Graphics/SwapChain.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include <iostream>

namespace Graphics
{
    SwapChain::SwapChain(const GraphicsDevice& device, HWND hwnd, uint32_t width, uint32_t height)
        : width_(width)
        , height_(height)
    {
        CreateSwapChain(device, hwnd);
        CreateRtvDescriptorHeap(device);
        CreateRenderTargets(device);

        std::cout << "[Graphics] スワップチェインと RTV ヒープの初期化が完了しました。\n";
    }

    void SwapChain::CreateSwapChain(const GraphicsDevice& device, HWND hwnd)
    {
        // スワップチェインの設定
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = width_;
        desc.Height = height_;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;       // 1画素あたり赤・緑・青・透明度それぞれ8bit (0~255)
        desc.Stereo = FALSE;
        desc.SampleDesc.Count = 1;                    // マルチサンプリングなし
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // レンダーターゲット（描画先）として使用
        desc.BufferCount = kBufferCount;              // 表画面と裏画面の計2枚
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Windows 10/11 で最も推奨される高速なフリップモード
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = 0;

        ComPtr<IDXGISwapChain1> swap_chain_1;
        ThrowIfFailed(
            device.GetFactory()->CreateSwapChainForHwnd(
                device.GetCommandQueue(), // コマンドキューを渡して画面切り替えを同期させる
                hwnd,
                &desc,
                nullptr,
                nullptr,
                &swap_chain_1
            ),
            "IDXGISwapChain1 の作成に失敗しました。"
        );

        // DXGI 1.4 の最新インターフェース (IDXGISwapChain4) へキャスト
        ThrowIfFailed(
            swap_chain_1.As(&swap_chain_),
            "IDXGISwapChain4 へのキャストに失敗しました。"
        );
    }

    void SwapChain::CreateRtvDescriptorHeap(const GraphicsDevice& device)
    {
        // ディスクリプタヒープ: GPU リソースの「名札（ディスクリプタ）」を並べて保管しておく名刺入れ
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.NumDescriptors = kBufferCount;            // バックバッファの枚数分だけ名札を用意
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   // レンダーターゲットビュー専用ヒープ
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダから直接参照しないため NONE

        ThrowIfFailed(
            device.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap_)),
            "RTV 用ディスクリプタヒープの作成に失敗しました。"
        );

        // GPU ハードウェアごとの名札1つ分のバイトサイズを取得（次項へのオフセット計算用）
        rtv_descriptor_size_ = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    void SwapChain::CreateRenderTargets(const GraphicsDevice& device)
    {
        // ヒープの先頭アドレスを取得
        auto rtv_handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();

        for (uint32_t i = 0; i < kBufferCount; ++i)
        {
            // スワップチェインからバックバッファのテクスチャリソースを取り出す
            ThrowIfFailed(
                swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_targets_[i])),
                "スワップチェインからのバックバッファ取得に失敗しました。"
            );

            // リソースに対して「これは描画先（RTV）ですよ」と名札をつけてヒープに書き込む
            device.GetDevice()->CreateRenderTargetView(render_targets_[i].Get(), nullptr, rtv_handle);

            // 次のバッファ用の名札アドレスへ進める
            rtv_handle.ptr += rtv_descriptor_size_;
        }
    }

    void SwapChain::Present(uint32_t sync_interval)
    {
        // 表画面と裏画面を入れ替えてディスプレイに表示
        ThrowIfFailed(
            swap_chain_->Present(sync_interval, 0),
            "画面表示 (Present) に失敗しました。"
        );
    }

    uint32_t SwapChain::GetCurrentBackBufferIndex() const noexcept
    {
        return swap_chain_->GetCurrentBackBufferIndex();
    }

    ID3D12Resource* SwapChain::GetCurrentRenderTarget() const noexcept
    {
        return render_targets_[GetCurrentBackBufferIndex()].Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentRtvHandle() const noexcept
    {
        auto handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(GetCurrentBackBufferIndex()) * rtv_descriptor_size_;
        return handle;
    }
}
