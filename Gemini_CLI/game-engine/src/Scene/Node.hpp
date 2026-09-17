#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <concepts>

namespace Graphics
{
    struct RenderItem;
}

namespace Scene
{
    class SceneTree;

    /**
     * @brief ゲーム世界のあらゆる要素の基礎となる基底クラス（Godot の Node を手本とした設計）
     *
     * 【Composite パターンの適用】
     * - 個々のオブジェクト（末端ノード）も、オブジェクトの集まり（親ノード）も、
     *   同じ `Node` インターフェースを通じて均一に扱うことができます。
     *
     * 【所有権のルール】
     * - 親ノード -> 子ノード: std::unique_ptr<Node>（親が子の排他所有権を持つ）
     * - 子ノード -> 親ノード: 生ポインタ Node*（所有権を持たない参照。循環参照を完全に防止）
     */
    class Node
    {
    public:
        explicit Node(std::string_view name = "Node");
        virtual ~Node();

        // コピー・ムーブの禁止（ツリーのポインタ整合性を保つため）
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&) = delete;
        Node& operator=(Node&&) = delete;

        // --- ライフサイクル仮想関数（派生クラスでオーバーライドして動作を実装） ---

        /**
         * @brief ノードがシーンツリーに参加した瞬間に呼ばれる
         */
        virtual void OnEnterTree() {}

        /**
         * @brief 自分およびすべての子ノードのツリー参加が完了した直後に呼ばれる（ボトムアップ）
         *
         * ※ 子ノードの準備がすべて整っていることが保証されているため、初期化コードはここに書きます。
         */
        virtual void OnReady() {}

        /**
         * @brief 毎フレーム呼び出される更新処理
         * @param delta 前フレームからの経過時間（秒）
         */
        virtual void OnProcess([[maybe_unused]] float delta) {}

        /**
         * @brief ノードがシーンツリーから除外される直前に呼ばれる（クリーンアップ用）
         */
        virtual void OnExitTree() {}

        /**
         * @brief 描画抽出フェーズ (Extract) で、描画可能なアイテムを収集する
         * @param out_items 描画パケットを追加するリスト
         */
        virtual void CollectRenderItems(std::vector<Graphics::RenderItem>& out_items);

        // --- ツリー構造の操作 ---

        /**
         * @brief 子ノードを追加する
         * @tparam T Node を継承したクラス型
         * @param child 追加する子ノードの unique_ptr
         * @return 追加された子ノードへの生ポインタ（所有権は親が持つ）
         */
        template <typename T>
        requires std::derived_from<T, Node>
        T* AddChild(std::unique_ptr<T> child)
        {
            T* raw_ptr = child.get();
            AddChildInternal(std::move(child));
            return raw_ptr;
        }

        /**
         * @brief 子ノードをツリーから切り離す（所有権を呼び出し元へ戻す）
         * @param child 切り離したい子ノード
         * @return 切り離された子ノードの unique_ptr（見つからなければ nullptr）
         */
        std::unique_ptr<Node> RemoveChild(Node* child);

        /**
         * @brief 安全な遅延削除を予約する（Godot の queue_free() と同等）
         *
         * ループの実行途中に delete されるクラッシュを防ぐため、
         * 削除フラグを立てておき、フレームの末尾で SceneTree が安全に一括解放します。
         */
        void QueueFree() noexcept;

        // --- ゲッター / セッター ---
        [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
        void SetName(std::string_view name) { name_ = name; }

        [[nodiscard]] Node* GetParent() const noexcept { return parent_; }
        [[nodiscard]] const std::vector<std::unique_ptr<Node>>& GetChildren() const noexcept { return children_; }
        [[nodiscard]] bool IsInsideTree() const noexcept { return is_inside_tree_; }
        [[nodiscard]] bool IsQueuedForDeletion() const noexcept { return is_queued_for_deletion_; }
        [[nodiscard]] SceneTree* GetTree() const noexcept { return tree_; }

        // --- SceneTree 内部から呼ばれる伝播メソッド ---
        void PropagateEnterTree(SceneTree* tree);
        void PropagateReady();
        void PropagateProcess(float delta);
        void PropagateExitTree();
        void CleanupQueuedFreeChildren();

    protected:
        std::string name_;                                     // ノードの識別名
        Node* parent_ = nullptr;                               // 親への生ポインタ（非所有）
        std::vector<std::unique_ptr<Node>> children_;          // 子ノード群（排他所有）
        SceneTree* tree_ = nullptr;                            // 所属する SceneTree
        bool is_inside_tree_ = false;                          // ツリーに参加しているか
        bool is_ready_called_ = false;                         // OnReady がすでに呼ばれたか
        bool is_queued_for_deletion_ = false;                  // 遅延削除予約フラグ

    private:
        void AddChildInternal(std::unique_ptr<Node> child);
    };
}
