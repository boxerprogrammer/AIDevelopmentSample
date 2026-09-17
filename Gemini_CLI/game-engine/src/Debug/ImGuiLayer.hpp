#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace Scene
{
    class SceneTree;
    class Node;
}

namespace Graphics
{
    struct RenderItem;
}

namespace Debug
{
    /**
     * @brief Dear ImGui を DirectX 12 / Win32 環境で管理・描画するデバッグ UI レイヤー
     *
     * 【このクラスの責務】
     * 1. ImGui コンテキストの初期化・終了管理（RAII）
     * 2. フォント描画用 SRV ディスクリプタヒープの作成と保持
     * 3. Win32 メッセージのフック処理（マウスクリックやキー入力を ImGui に伝達）
     * 4. 毎フレームのデバッグ UI 描画（シーンヒエラルキー、インスペクタ、パフォーマンス統計）
     * 5. DirectX 12 コマンドリストへの描画コマンド発行
     */
    class ImGuiLayer
    {
    public:
        /**
         * @brief コンストラクタ: ImGui と DirectX 12 / Win32 バックエンドを初期化
         * @param hwnd 対象ウィンドウハンドル
         * @param device DirectX 12 デバイス
         * @param command_queue コマンドキュー（フォントテクスチャ転送等の GPU 実行に必須）
         * @param buffer_count スワップチェインのバックバッファ数（通常 2）
         * @param rtv_format レンダーターゲットのピクセルフォーマット
         */
        ImGuiLayer(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* command_queue, uint32_t buffer_count, DXGI_FORMAT rtv_format);

        /**
         * @brief デストラクタ: ImGui の安全なシャットダウンとリソース解放
         */
        ~ImGuiLayer();

        ImGuiLayer(const ImGuiLayer&) = delete;
        ImGuiLayer& operator=(const ImGuiLayer&) = delete;
        ImGuiLayer(ImGuiLayer&&) = delete;
        ImGuiLayer& operator=(ImGuiLayer&&) = delete;

        /**
         * @brief Win32 メッセージ処理用ハンドラ（Window クラスの SetCustomWndProcHandler に渡す）
         * @return ImGui が入力を消費した場合に LRESULT を返す（通常は 0 以外）
         */
        std::optional<LRESULT> HandleWin32Message(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

        /**
         * @brief フレーム開始処理（ImGui の NewFrame を発行）
         */
        void BeginFrame();

        /**
         * @brief 教育用エンジン向けデバッグ UI の構築（ヒエラルキー、インスペクタ、統計情報）
         * @param scene_tree 操作対象のシーンツリー
         * @param delta_time 前フレームからの経過時間（秒）
         * @param render_items_count 描画対象の RenderItem 数
         */
        void RenderDebugUI(Scene::SceneTree& scene_tree, float delta_time, size_t render_items_count);

        /**
         * @brief ImGui の描画コマンドを DirectX 12 コマンドリストに記録
         * @param command_list 描画コマンドリスト
         */
        void Render(ID3D12GraphicsCommandList* command_list);

        // 外部から選択ノードを設定
        void SelectNode(Scene::Node* node) noexcept { selected_node_ = node; }
        [[nodiscard]] Scene::Node* GetSelectedNode() const noexcept { return selected_node_; }

    private:
        /**
         * @brief シーンヒエラルキー（ノードの木構造）を再帰的に ImGui::TreeNode で描画
         */
        void DrawNodeHierarchy(Scene::Node* node);

        /**
         * @brief 選択中ノードのパラメータをリアルタイム編集するインスペクタを描画
         */
        void DrawInspector();

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_heap_; // ImGui フォントテクスチャ用 SRV ヒープ
        Scene::Node* selected_node_ = nullptr;                  // 現在インスペクタで選択中のノード
        bool show_demo_window_ = false;                         // ImGui デモウィンドウ表示フラグ
    };
}
