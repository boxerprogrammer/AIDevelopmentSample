#include "Scene/Node.hpp"
#include "Graphics/RenderItem.hpp"
#include <algorithm>
#include <iostream>

namespace Scene
{
    Node::Node(std::string_view name)
        : name_(name)
    {
    }

    Node::~Node()
    {
        // 破棄時にツリー内に残っていれば、安全に OnExitTree を呼び出しておく
        if (is_inside_tree_)
        {
            PropagateExitTree();
        }
    }

    void Node::AddChildInternal(std::unique_ptr<Node> child)
    {
        if (!child)
        {
            return;
        }

        if (child->parent_)
        {
            std::cerr << "[Warning] ノード '" << child->GetName() 
                      << "' はすでに別の親を持っています。追加できません。\n";
            return;
        }

        child->parent_ = this;

        // 親がすでにツリーに参加している場合、追加された子ノードも即座にツリーへ参加させる
        if (is_inside_tree_)
        {
            child->PropagateEnterTree(tree_);
            child->PropagateReady();
        }

        children_.push_back(std::move(child));
    }

    std::unique_ptr<Node> Node::RemoveChild(Node* child)
    {
        if (!child)
        {
            return nullptr;
        }

        auto it = std::find_if(
            children_.begin(),
            children_.end(),
            [child](const std::unique_ptr<Node>& ptr) { return ptr.get() == child; }
        );

        if (it != children_.end())
        {
            std::unique_ptr<Node> removed_child = std::move(*it);
            children_.erase(it);

            // ツリーから離脱させる
            removed_child->PropagateExitTree();
            removed_child->parent_ = nullptr;

            return removed_child;
        }

        return nullptr;
    }

    void Node::QueueFree() noexcept
    {
        is_queued_for_deletion_ = true;
    }

    void Node::PropagateEnterTree(SceneTree* tree)
    {
        tree_ = tree;
        is_inside_tree_ = true;

        // 自身の OnEnterTree を呼び出す
        OnEnterTree();

        // すべての子ノードへ再帰的に伝播
        for (const auto& child : children_)
        {
            child->PropagateEnterTree(tree);
        }
    }

    void Node::PropagateReady()
    {
        // 【Godot の流儀: ボトムアップの OnReady】
        // まず子ノードの OnReady を先にすべて完了させてから、最後に親の OnReady を呼ぶ
        // これにより、親ノードは「子の初期化が完全に終わっている状態」で処理を開始できる
        for (const auto& child : children_)
        {
            child->PropagateReady();
        }

        if (!is_ready_called_)
        {
            is_ready_called_ = true;
            OnReady();
        }
    }

    void Node::PropagateProcess(float delta)
    {
        // 削除予約されているノードは更新処理をスキップ
        if (!is_queued_for_deletion_)
        {
            OnProcess(delta);
        }

        // 子ノードへ再帰的に更新を伝播
        for (const auto& child : children_)
        {
            child->PropagateProcess(delta);
        }
    }

    void Node::PropagateExitTree()
    {
        // 子ノードから先にツリーを抜ける
        for (const auto& child : children_)
        {
            child->PropagateExitTree();
        }

        if (is_inside_tree_)
        {
            OnExitTree();
            is_inside_tree_ = false;
            tree_ = nullptr;
        }
    }

    void Node::CleanupQueuedFreeChildren()
    {
        // 1. まず孫ノード以下の掃除を再帰的に実行
        for (const auto& child : children_)
        {
            child->CleanupQueuedFreeChildren();
        }

        // 2. 自身の子ノードの中で QueueFree されたものを安全に破棄
        std::erase_if(children_, [](const std::unique_ptr<Node>& child) {
            if (child->IsQueuedForDeletion())
            {
                child->PropagateExitTree();
                // true を返すと vector から取り除かれ、unique_ptr の所有権が切れて安全に delete される
                return true;
            }
            return false;
        });
    }

    void Node::CollectRenderItems(std::vector<Graphics::RenderItem>& out_items)
    {
        // 基底クラス Node では自身は描画物を持たず、子ノードへ収集を再帰委譲する
        for (const auto& child : children_)
        {
            if (!child->IsQueuedForDeletion())
            {
                child->CollectRenderItems(out_items);
            }
        }
    }
}
