#include "Scene/Node3D.hpp"

namespace Scene
{
    Node3D::Node3D(std::string_view name)
        : Node(name)
    {
    }

    DirectX::XMMATRIX Node3D::GetWorldMatrix() const noexcept
    {
        const DirectX::XMMATRIX local = transform_.GetLocalMatrix();

        // 親ノードが存在し、かつそれが Node3D であれば、親のワールド行列を掛け合わせる
        if (parent_)
        {
            if (const auto* parent_3d = dynamic_cast<const Node3D*>(parent_))
            {
                // 行列の掛け算: 子のローカル変換 × 親のワールド変換
                return local * parent_3d->GetWorldMatrix();
            }
        }

        return local;
    }
}
