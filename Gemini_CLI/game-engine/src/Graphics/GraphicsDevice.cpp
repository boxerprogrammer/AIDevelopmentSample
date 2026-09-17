#include "Graphics/GraphicsDevice.hpp"
#include <iostream>

namespace Graphics
{
    GraphicsDevice::GraphicsDevice(bool enable_debug_layer)
    {
        if (enable_debug_layer)
        {
            InitializeDebugLayer();
        }

        CreateDeviceAndFactory();
        CreateCommandQueue();
        CreateCommandListAndAllocator();
        CreateFence();

        std::cout << "[Graphics] DirectX 12 デバイス・キュー・コマンドリストの初期化が完了しました。\n";
    }

    GraphicsDevice::~GraphicsDevice()
    {
        // アプリケーション終了時、GPU がまだ処理中かもしれないので完了を確実に待機する
        try
        {
            WaitForGpu();
        }
        catch (...)
        {
            // デストラクタ内での例外送出を抑制
        }

        if (fence_event_)
        {
            CloseHandle(fence_event_);
            fence_event_ = nullptr;
        }
    }

    void GraphicsDevice::InitializeDebugLayer()
    {
        // デバッグレイヤー: 不正な API 呼び出しやバリアのミスを検知して詳細なログを出力する機能
        ComPtr<ID3D12Debug> debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
        {
            debug_controller->EnableDebugLayer();
            std::cout << "[Graphics] DirectX 12 デバッグレイヤーを有効化しました。\n";
        }
    }

    void GraphicsDevice::CreateDeviceAndFactory()
    {
        UINT create_factory_flags = 0;
#if defined(_DEBUG)
        create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        ThrowIfFailed(
            CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory_)),
            "DXGI Factory の作成に失敗しました。"
        );

        // 高性能な専用 GPU (NVIDIA / AMD 等) を優先して探す
        ComPtr<IDXGIAdapter1> selected_adapter;
        for (UINT i = 0;
             dxgi_factory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&selected_adapter)) != DXGI_ERROR_NOT_FOUND;
             ++i)
        {
            DXGI_ADAPTER_DESC1 desc{};
            selected_adapter->GetDesc1(&desc);

            // ソフトウェアレンダラー (WARP) をスキップして物理 GPU を優先
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            // D3D12 対応の Feature Level 12.0 をサポートしているか確認し、デバイスを生成
            if (SUCCEEDED(D3D12CreateDevice(selected_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_))))
            {
                std::wcout << L"[Graphics] 使用する GPU アダプタ: " << desc.Description << L"\n";
                break;
            }
        }

        if (!device_)
        {
            throw std::runtime_error("Direct3D 12 (Feature Level 12.0) をサポートする GPU が見つかりませんでした。");
        }
    }

    void GraphicsDevice::CreateCommandQueue()
    {
        // コマンドキュー: GPU に対して「この順序で伝票を実行してね」と渡す注文投入口
        D3D12_COMMAND_QUEUE_DESC queue_desc{};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 直接描画を行う標準のキュー
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(
            device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)),
            "ID3D12CommandQueue の作成に失敗しました。"
        );
    }

    void GraphicsDevice::CreateCommandListAndAllocator()
    {
        // コマンドアロケータ: コマンドリストが命令を書き留めるための「伝票用紙（メモリ領域）」
        ThrowIfFailed(
            device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_)),
            "ID3D12CommandAllocator の作成に失敗しました。"
        );

        // コマンドリスト: 実際の命令（クリア、描画など）を書き込む「伝票」
        ThrowIfFailed(
            device_->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                command_allocator_.Get(),
                nullptr, // 初期パイプラインステート（後ほど設定）
                IID_PPV_ARGS(&command_list_)
            ),
            "ID3D12GraphicsCommandList の作成に失敗しました。"
        );

        // 生成直後のコマンドリストは「記録受付中 (Open)」状態なので、初期化の段階では一度閉じておく
        ThrowIfFailed(command_list_->Close(), "初期コマンドリストのクローズに失敗しました。");
        is_command_list_open_ = false;
    }

    void GraphicsDevice::CreateFence()
    {
        // フェンス: CPU と GPU の進行度を数字 (uint64_t) で管理・同期する呼び出しベル
        fence_value_ = 0;
        ThrowIfFailed(
            device_->CreateFence(fence_value_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)),
            "ID3D12Fence の作成に失敗しました。"
        );

        // GPU が指定値に達したときに起こしてもらうための OS イベントを作成
        fence_event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fence_event_)
        {
            throw std::runtime_error("フェンス用 Win32 イベントの作成に失敗しました。");
        }
    }

    void GraphicsDevice::BeginCommandList()
    {
        // アロケータのメモリをリセット（※GPU が前フレームの伝票を読み終えていることが前提）
        ThrowIfFailed(command_allocator_->Reset(), "コマンドアロケータのリセットに失敗しました。");

        // コマンドリストを記録受付中（Open）に戻す
        ThrowIfFailed(command_list_->Reset(command_allocator_.Get(), nullptr), "コマンドリストのリセットに失敗しました。");
        is_command_list_open_ = true;
    }

    void GraphicsDevice::ExecuteCommandList()
    {
        if (is_command_list_open_)
        {
            // 伝票を提出する前に「書き終わり（Close）」を宣言する
            ThrowIfFailed(command_list_->Close(), "コマンドリストのクローズに失敗しました。");
            is_command_list_open_ = false;
        }

        // コマンドキューに伝票を投入し、GPU に実行させる
        ID3D12CommandList* const command_lists[] = { command_list_.Get() };
        command_queue_->ExecuteCommandLists(1, command_lists);
    }

    void GraphicsDevice::WaitForGpu()
    {
        // 1. フェンスの目標値をインクリメント
        ++fence_value_;

        // 2. コマンドキューに「これまでの仕事が終わったら、フェンスの値を fence_value_ に書き換えてね」と命令を送る
        ThrowIfFailed(
            command_queue_->Signal(fence_.Get(), fence_value_),
            "コマンドキューへのフェンス Signal 送信に失敗しました。"
        );

        // 3. GPU がまだ目標値に達していなければ、イベントをセットして CPU スレッドを待機させる
        if (fence_->GetCompletedValue() < fence_value_)
        {
            ThrowIfFailed(
                fence_->SetEventOnCompletion(fence_value_, fence_event_),
                "フェンスイベントの登録に失敗しました。"
            );
            WaitForSingleObject(fence_event_, INFINITE);
        }
    }
}
