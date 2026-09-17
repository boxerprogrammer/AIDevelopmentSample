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

    DirectX::XMFLOAT3 Node3D::GetForward() const noexcept
    {
        // 行優先のワールド変換行列において、Row 2 (Z軸) が前方ベクトルを表す
        const DirectX::XMMATRIX world = GetWorldMatrix();
        const DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(world.r[2]);
        DirectX::XMFLOAT3 result{};
        DirectX::XMStoreFloat3(&result, forward);
        return result;
    }

    DirectX::XMFLOAT3 Node3D::GetRight() const noexcept
    {
        // 行優先のワールド変換行列において、Row 0 (X軸) が右方向ベクトルを表す
        const DirectX::XMMATRIX world = GetWorldMatrix();
        const DirectX::XMVECTOR right = DirectX::XMVector3Normalize(world.r[0]);
        DirectX::XMFLOAT3 result{};
        DirectX::XMStoreFloat3(&result, right);
        return result;
    }

    DirectX::XMFLOAT3 Node3D::GetUp() const noexcept
    {
        // 行優先のワールド変換行列において、Row 1 (Y軸) が上方向ベクトルを表す
        const DirectX::XMMATRIX world = GetWorldMatrix();
        const DirectX::XMVECTOR up = DirectX::XMVector3Normalize(world.r[1]);
        DirectX::XMFLOAT3 result{};
        DirectX::XMStoreFloat3(&result, up);
        return result;
    }
}
