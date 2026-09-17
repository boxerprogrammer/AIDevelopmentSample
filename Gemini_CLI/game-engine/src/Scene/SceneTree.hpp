#pragma once

#include "Scene/Node.hpp"
#include <memory>

namespace Scene
{
    /**
     * @brief シーンツリー全体を統括・駆動する最上位マネージャクラス（Godot の SceneTree 相当）
     *
     * 【このクラスの責務】
     * 1. ツリーの最上位であるルートノード (Root Node) の管理
     * 2. 毎フレームの Process(delta) のツリー全体への伝播
     * 3. フレーム末尾における遅延削除 (QueueFree) されたノードの安全な一括回収・解放
     */
    class SceneTree
    {
    public:
        SceneTree();
        ~SceneTree();

        SceneTree(const SceneTree&) = delete;
        SceneTree& operator=(const SceneTree&) = delete;
        SceneTree(SceneTree&&) noexcept = default;
        SceneTree& operator=(SceneTree&&) noexcept = default;

        /**
         * @brief ルートノードを設定または差し替える
         * @param root 新しいルートノード（所有権を SceneTree が引き受ける）
         */
        void SetRootNode(std::unique_ptr<Node> root);

        /**
         * @brief 毎フレーム呼び出されるメインループ更新
         * @param delta 前フレームからの経過時間（秒）
         */
        void Process(float delta);

        /**
         * @brief フレームの末尾で呼び出し、QueueFree されたノードを一括削除する
         */
        void Cleanup();

        /**
         * @brief 描画抽出フェーズ (Extract): ツリー内の描画可能ノードから描画パケットを一括収集する
         */
        [[nodiscard]] std::vector<Graphics::RenderItem> ExtractRenderItems() const;

        [[nodiscard]] Node* GetRootNode() const noexcept { return root_node_.get(); }

    private:
        std::unique_ptr<Node> root_node_; // シーンツリーの最上位ノード
    };
}
