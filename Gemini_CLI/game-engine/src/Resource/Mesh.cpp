#include "Resource/Mesh.hpp"
#include <vector>

namespace Resource
{
    Mesh::Mesh(std::unique_ptr<Graphics::VertexBuffer> vertex_buffer,
               std::unique_ptr<Graphics::IndexBuffer> index_buffer,
               std::string_view path)
        : Resource(path)
        , vertex_buffer_(std::move(vertex_buffer))
        , index_buffer_(std::move(index_buffer))
    {
    }

    std::shared_ptr<Mesh> Mesh::CreateCube(ID3D12Device* device, float size)
    {
        const float h = size * 0.5f;

        // サイコロ型カラーキューブ（24 頂点: 6面 × 4頂点）
        // 各面に外向きの法線ベクトル (normal) を設定し、ライティング計算を正確に行います
        const std::vector<Graphics::Vertex> vertices = {
            // 前面 (赤) - Z = -h, Normal = (0, 0, -1)
            { { -h, -h, -h }, {  0.0f,  0.0f, -1.0f }, { 0.9f, 0.2f, 0.2f, 1.0f }, { 0.0f, 1.0f } },
            { { -h,  h, -h }, {  0.0f,  0.0f, -1.0f }, { 0.9f, 0.2f, 0.2f, 1.0f }, { 0.0f, 0.0f } },
            { {  h,  h, -h }, {  0.0f,  0.0f, -1.0f }, { 0.9f, 0.2f, 0.2f, 1.0f }, { 1.0f, 0.0f } },
            { {  h, -h, -h }, {  0.0f,  0.0f, -1.0f }, { 0.9f, 0.2f, 0.2f, 1.0f }, { 1.0f, 1.0f } },

            // 背面 (シアン) - Z = +h, Normal = (0, 0, 1)
            { {  h, -h,  h }, {  0.0f,  0.0f,  1.0f }, { 0.2f, 0.8f, 0.9f, 1.0f }, { 0.0f, 1.0f } },
            { {  h,  h,  h }, {  0.0f,  0.0f,  1.0f }, { 0.2f, 0.8f, 0.9f, 1.0f }, { 0.0f, 0.0f } },
            { { -h,  h,  h }, {  0.0f,  0.0f,  1.0f }, { 0.2f, 0.8f, 0.9f, 1.0f }, { 1.0f, 0.0f } },
            { { -h, -h,  h }, {  0.0f,  0.0f,  1.0f }, { 0.2f, 0.8f, 0.9f, 1.0f }, { 1.0f, 1.0f } },

            // 上面 (緑) - Y = +h, Normal = (0, 1, 0)
            { { -h,  h, -h }, {  0.0f,  1.0f,  0.0f }, { 0.2f, 0.9f, 0.3f, 1.0f }, { 0.0f, 1.0f } },
            { { -h,  h,  h }, {  0.0f,  1.0f,  0.0f }, { 0.2f, 0.9f, 0.3f, 1.0f }, { 0.0f, 0.0f } },
            { {  h,  h,  h }, {  0.0f,  1.0f,  0.0f }, { 0.2f, 0.9f, 0.3f, 1.0f }, { 1.0f, 0.0f } },
            { {  h,  h, -h }, {  0.0f,  1.0f,  0.0f }, { 0.2f, 0.9f, 0.3f, 1.0f }, { 1.0f, 1.0f } },

            // 下面 (黄) - Y = -h, Normal = (0, -1, 0)
            { { -h, -h,  h }, {  0.0f, -1.0f,  0.0f }, { 0.9f, 0.9f, 0.2f, 1.0f }, { 0.0f, 1.0f } },
            { { -h, -h, -h }, {  0.0f, -1.0f,  0.0f }, { 0.9f, 0.9f, 0.2f, 1.0f }, { 0.0f, 0.0f } },
            { {  h, -h, -h }, {  0.0f, -1.0f,  0.0f }, { 0.9f, 0.9f, 0.2f, 1.0f }, { 1.0f, 0.0f } },
            { {  h, -h,  h }, {  0.0f, -1.0f,  0.0f }, { 0.9f, 0.9f, 0.2f, 1.0f }, { 1.0f, 1.0f } },

            // 左面 (青) - X = -h, Normal = (-1, 0, 0)
            { { -h, -h,  h }, { -1.0f,  0.0f,  0.0f }, { 0.2f, 0.4f, 0.9f, 1.0f }, { 0.0f, 1.0f } },
            { { -h,  h,  h }, { -1.0f,  0.0f,  0.0f }, { 0.2f, 0.4f, 0.9f, 1.0f }, { 0.0f, 0.0f } },
            { { -h,  h, -h }, { -1.0f,  0.0f,  0.0f }, { 0.2f, 0.4f, 0.9f, 1.0f }, { 1.0f, 0.0f } },
            { { -h, -h, -h }, { -1.0f,  0.0f,  0.0f }, { 0.2f, 0.4f, 0.9f, 1.0f }, { 1.0f, 1.0f } },

            // 右面 (マゼンタ) - X = +h, Normal = (1, 0, 0)
            { {  h, -h, -h }, {  1.0f,  0.0f,  0.0f }, { 0.9f, 0.2f, 0.8f, 1.0f }, { 0.0f, 1.0f } },
            { {  h,  h, -h }, {  1.0f,  0.0f,  0.0f }, { 0.9f, 0.2f, 0.8f, 1.0f }, { 0.0f, 0.0f } },
            { {  h,  h,  h }, {  1.0f,  0.0f,  0.0f }, { 0.9f, 0.2f, 0.8f, 1.0f }, { 1.0f, 0.0f } },
            { {  h, -h,  h }, {  1.0f,  0.0f,  0.0f }, { 0.9f, 0.2f, 0.8f, 1.0f }, { 1.0f, 1.0f } },
        };

        // 時計回りのインデックス（36 インデックス: 6面 × 2三角形 × 3頂点）
        const std::vector<uint16_t> indices = {
            // 前面
            0, 1, 2, 0, 2, 3,
            // 背面
            4, 5, 6, 4, 6, 7,
            // 上面
            8, 9, 10, 8, 10, 11,
            // 下面
            12, 13, 14, 12, 14, 15,
            // 左面
            16, 17, 18, 16, 18, 19,
            // 右面
            20, 21, 22, 20, 22, 23,
        };

        auto vb = std::make_unique<Graphics::VertexBuffer>(device, vertices);
        auto ib = std::make_unique<Graphics::IndexBuffer>(device, indices);

        return std::make_shared<Mesh>(std::move(vb), std::move(ib), "builtin://Mesh/Cube");
    }

    std::shared_ptr<Mesh> Mesh::CreatePlane(ID3D12Device* device, float width, float depth)
    {
        const float hw = width * 0.5f;
        const float hd = depth * 0.5f;

        // XZ 平面メッシュ（4 頂点、上向き法線 (0, 1, 0)、ダークグレー）
        const std::vector<Graphics::Vertex> vertices = {
            { { -hw, 0.0f, -hd }, { 0.0f, 1.0f, 0.0f }, { 0.4f, 0.4f, 0.45f, 1.0f }, {  0.0f, 10.0f } },
            { { -hw, 0.0f,  hd }, { 0.0f, 1.0f, 0.0f }, { 0.4f, 0.4f, 0.45f, 1.0f }, {  0.0f,  0.0f } },
            { {  hw, 0.0f,  hd }, { 0.0f, 1.0f, 0.0f }, { 0.4f, 0.4f, 0.45f, 1.0f }, { 10.0f,  0.0f } },
            { {  hw, 0.0f, -hd }, { 0.0f, 1.0f, 0.0f }, { 0.4f, 0.4f, 0.45f, 1.0f }, { 10.0f, 10.0f } },
        };

        const std::vector<uint16_t> indices = {
            0, 1, 2, 0, 2, 3
        };

        auto vb = std::make_unique<Graphics::VertexBuffer>(device, vertices);
        auto ib = std::make_unique<Graphics::IndexBuffer>(device, indices);

        return std::make_shared<Mesh>(std::move(vb), std::move(ib), "builtin://Mesh/Plane");
    }
}
