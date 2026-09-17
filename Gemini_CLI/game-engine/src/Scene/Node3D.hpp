#pragma once

#include "Scene/Node.hpp"
#include "Scene/Transform3D.hpp"

namespace Scene
{
    /**
     * @brief 3D 空間内に位置・回転・拡縮を持つノード（Godot の Node3D 相当）
     *
     * 【親子関係とワールド行列の自動計算】
     * - 親ノードが Node3D である場合、自身のローカル変換行列に親のワールド行列を再帰的に掛け合わせます。
     * - これにより、太陽の周りを回る地球、地球の周りを回る月のような階層構造が自然に実現します。
     */
    class Node3D : public Node
    {
    public:
        explicit Node3D(std::string_view name = "Node3D");
        ~Node3D() override = default;

        /**
         * @brief 親ノードの姿勢を反映した最終的なワールド変換行列を取得
         */
        [[nodiscard]] DirectX::XMMATRIX GetWorldMatrix() const noexcept;

        // トランスフォームへのアクセス
        [[nodiscard]] Transform3D& GetTransform() noexcept { return transform_; }
        [[nodiscard]] const Transform3D& GetTransform() const noexcept { return transform_; }

        // 便利なショートカット関数群
        void SetPosition(float x, float y, float z) noexcept { transform_.SetPosition(x, y, z); }
        void SetPosition(const DirectX::XMFLOAT3& pos) noexcept { transform_.SetPosition(pos); }
        void Translate(float dx, float dy, float dz) noexcept { transform_.Translate(dx, dy, dz); }

        void SetRotation(float pitch, float yaw, float roll) noexcept { transform_.SetRotation(pitch, yaw, roll); }
        void Rotate(float dpitch, float dyaw, float droll) noexcept { transform_.Rotate(dpitch, dyaw, droll); }

        void SetScale(float s) noexcept { transform_.SetScale(s); }
        void SetScale(float x, float y, float z) noexcept { transform_.SetScale(x, y, z); }

    protected:
        Transform3D transform_; // ローカル変換情報
    };
}
