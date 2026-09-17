#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <cstdint>

namespace Graphics
{
    /**
     * @brief DirectX 12 の中核基盤（GPU デバイス・キュー・同期フェンス）を管理するクラス
     *
     * 【このクラスの責務】
     * 1. GPU (ID3D12Device) の検出・初期化
     * 2. 命令受付キュー (ID3D12CommandQueue) の管理
     * 3. コマンド記録用のメモリ (ID3D12CommandAllocator) と伝票 (ID3D12GraphicsCommandList) の管理
     * 4. CPU と GPU の足並みを揃えるフェンス (ID3D12Fence) による同期
     */
    class GraphicsDevice
    {
    public:
        /**
         * @brief グラフィックスデバイスを初期化する
         * @param enable_debug_layer デバッグ時に詳細な警告・エラーを出力するデバッグレイヤーを有効化するか
         */
        explicit GraphicsDevice(bool enable_debug_layer = true);

        /**
         * @brief デストラクタ: GPU の作業完了を待機してからリソースを破棄
         */
        ~GraphicsDevice();

        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;
        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;

        /**
         * @brief 新しいフレームのコマンド記録を開始する
         *
         * 前のフレームで使ったアロケータ（用紙トレイ）とコマンドリスト（伝票）をリセットして再利用します。
         */
        void BeginCommandList();

        /**
         * @brief 記録したコマンドリストを閉じ、GPU のキューに送信して実行を開始する
         */
        void ExecuteCommandList();

        /**
         * @brief GPU が現在キューに入っているすべての作業を終えるまで、CPU を待機させる
         *
         * フェンスの値をインクリメントしてキューにシグナルを送り、その値に達するまで待機します。
         */
        void WaitForGpu();

        // ゲッター関数群
        [[nodiscard]] ID3D12Device* GetDevice() const noexcept { return device_.Get(); }
        [[nodiscard]] ID3D12CommandQueue* GetCommandQueue() const noexcept { return command_queue_.Get(); }
        [[nodiscard]] ID3D12GraphicsCommandList* GetCommandList() const noexcept { return command_list_.Get(); }
        [[nodiscard]] IDXGIFactory4* GetFactory() const noexcept { return dxgi_factory_.Get(); }

    private:
        void InitializeDebugLayer();
        void CreateDeviceAndFactory();
        void CreateCommandQueue();
        void CreateCommandListAndAllocator();
        void CreateFence();

    private:
        ComPtr<IDXGIFactory6> dxgi_factory_;                // GPU アダプタの列挙やスワップチェイン作成を行う工場
        ComPtr<ID3D12Device> device_;                       // GPU との対話窓口（リソースやパイプラインの生成担当）
        ComPtr<ID3D12CommandQueue> command_queue_;          // GPU への命令提出口（厨房への注文伝票投入口）
        ComPtr<ID3D12CommandAllocator> command_allocator_;  // コマンドリストが命令を書き込むメモリ領域（伝票用紙）
        ComPtr<ID3D12GraphicsCommandList> command_list_;    // 描画・バリア命令を記録する伝票本体

        // CPU と GPU の同期機構（料理完成の呼び出しベル）
        ComPtr<ID3D12Fence> fence_;                         // フェンスオブジェクト
        HANDLE fence_event_ = nullptr;                      // フェンス到達時にシグナルを受け取る Win32 イベント
        uint64_t fence_value_ = 0;                          // フェンスの現在値（1フレーム進むごとに +1）
        bool is_command_list_open_ = false;                 // コマンドリストが現在記録受付中かどうかのフラグ
    };
}
