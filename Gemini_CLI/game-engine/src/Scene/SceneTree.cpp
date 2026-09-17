#include "Scene/SceneTree.hpp"
#include "Graphics/RenderItem.hpp"
#include <iostream>

namespace Scene
{
    SceneTree::SceneTree()
    {
        // 最上位のデフォルトルートノードを作成
        SetRootNode(std::make_unique<Node>("Root"));
    }

    SceneTree::~SceneTree()
    {
        if (root_node_)
        {
            root_node_->PropagateExitTree();
            root_node_.reset();
        }
    }

    void SceneTree::SetRootNode(std::unique_ptr<Node> root)
    {
        if (root_node_)
        {
            root_node_->PropagateExitTree();
        }

        root_node_ = std::move(root);

        if (root_node_)
        {
            root_node_->PropagateEnterTree(this);
            root_node_->PropagateReady();
        }
    }

    void SceneTree::Process(float delta)
    {
        if (root_node_)
        {
            root_node_->PropagateProcess(delta);
        }
    }

    void SceneTree::Cleanup()
    {
        if (!root_node_)
        {
            return;
        }

        // 子ノード以下の遅延削除を再帰的に実行
        root_node_->CleanupQueuedFreeChildren();

        // ルートノード自身が削除予約されている場合は破棄
        if (root_node_->IsQueuedForDeletion())
        {
            root_node_->PropagateExitTree();
            root_node_.reset();
        }
    }

    std::vector<Graphics::RenderItem> SceneTree::ExtractRenderItems() const
    {
        std::vector<Graphics::RenderItem> items;
        if (root_node_)
        {
            root_node_->CollectRenderItems(items);
        }
        return items;
    }
}
