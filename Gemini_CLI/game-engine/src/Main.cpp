#include "Core/Window.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/SwapChain.hpp"
#include "Graphics/DepthBuffer.hpp"
#include "Graphics/Shader.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/VertexBuffer.hpp"
#include "Graphics/IndexBuffer.hpp"
#include "Graphics/ConstantBuffer.hpp"
#include "Graphics/RenderItem.hpp"

#include "Scene/SceneTree.hpp"
#include "Scene/Node3D.hpp"
#include "Scene/Camera3D.hpp"
#include "Scene/MeshInstance3D.hpp"

#include <DirectXMath.h>
#include <iostream>
#include <exception>
#include <vector>
#include <chrono>

// 定数バッファ用データ構造体
struct TransformData
{
    DirectX::XMFLOAT4X4 mvp; // 4x4 の MVP 行列
};

/**
 * @brief 自転する動作をカプセル化したカスタム 3D ノード（Godot のスクリプトアタッチと同等の役割）
 */
class RotatorNode : public Scene::Node3D
{
public:
    using Node3D::Node3D;

    /**
     * @brief 毎フレームの更新処理（Godot の _process(delta) 相当）
     */
    void OnProcess(float delta) override
    {
        // 時間差分 delta を使って、フレームレートに依存しない滑らかな自転を行う
        Rotate(delta * 0.5f, delta * 1.0f, 0.0f);
    }
};

int main()
{
    // 日本語文字化け防止
    std::setlocale(LC_ALL, ".UTF-8");

    std::cout << "========================================\n";
    std::cout << "  EduGameEngine - 教育用ゲームエンジン  \n";
    std::cout << "  Phase 2: Godot 風シーン・ノード構造   \n";
    std::cout << "========================================\n\n";

    try
    {
        // 1. ウィンドウの生成
        constexpr uint32_t window_width = 1280;
        constexpr uint32_t window_height = 720;
        Core::Window window(L"EduGameEngine [DirectX 12 / C++20] - Phase 2: Scene & Node Tree", window_width, window_height);

        // 2. DirectX 12 グラフィックス基盤
        Graphics::GraphicsDevice graphics_device(/*enable_debug_layer=*/true);
        Graphics::SwapChain swap_chain(graphics_device, window.GetHwnd(), window_width, window_height);
        Graphics::DepthBuffer depth_buffer(graphics_device.GetDevice(), window_width, window_height);

        // 3. パイプライン・シェーダの準備
        Graphics::Shader vertex_shader("shaders/SimpleTriangle.hlsl", "VSMain", Graphics::ShaderStage::Vertex);
        Graphics::Shader pixel_shader("shaders/SimpleTriangle.hlsl", "PSMain", Graphics::ShaderStage::Pixel);
        Graphics::Pipeline pipeline(graphics_device.GetDevice(), vertex_shader, pixel_shader);

        // 4. メッシュデータ（3D キューブ）の作成
        const std::vector<Graphics::Vertex> cube_vertices = {
            // 前面 (赤)
            { { -0.5f, -0.5f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },
            { {  0.5f, -0.5f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },
            // 背面 (シアン)
            { {  0.5f, -0.5f,  0.5f }, { 0.2f, 0.8f, 0.9f, 1.0f } },
            { {  0.5f,  0.5f,  0.5f }, { 0.2f, 0.8f, 0.9f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, { 0.2f, 0.8f, 0.9f, 1.0f } },
            { { -0.5f, -0.5f,  0.5f }, { 0.2f, 0.8f, 0.9f, 1.0f } },
            // 上面 (緑)
            { { -0.5f,  0.5f, -0.5f }, { 0.2f, 0.9f, 0.3f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, { 0.2f, 0.9f, 0.3f, 1.0f } },
            { {  0.5f,  0.5f,  0.5f }, { 0.2f, 0.9f, 0.3f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, { 0.2f, 0.9f, 0.3f, 1.0f } },
            // 下面 (黄)
            { { -0.5f, -0.5f,  0.5f }, { 0.9f, 0.9f, 0.2f, 1.0f } },
            { { -0.5f, -0.5f, -0.5f }, { 0.9f, 0.9f, 0.2f, 1.0f } },
            { {  0.5f, -0.5f, -0.5f }, { 0.9f, 0.9f, 0.2f, 1.0f } },
            { {  0.5f, -0.5f,  0.5f }, { 0.9f, 0.9f, 0.2f, 1.0f } },
            // 左面 (青)
            { { -0.5f, -0.5f,  0.5f }, { 0.2f, 0.4f, 0.9f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, { 0.2f, 0.4f, 0.9f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, { 0.2f, 0.4f, 0.9f, 1.0f } },
            { { -0.5f, -0.5f, -0.5f }, { 0.2f, 0.4f, 0.9f, 1.0f } },
            // 右面 (マゼンタ)
            { {  0.5f, -0.5f, -0.5f }, { 0.9f, 0.2f, 0.8f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, { 0.9f, 0.2f, 0.8f, 1.0f } },
            { {  0.5f,  0.5f,  0.5f }, { 0.9f, 0.2f, 0.8f, 1.0f } },
            { {  0.5f, -0.5f,  0.5f }, { 0.9f, 0.2f, 0.8f, 1.0f } }
        };

        const std::vector<uint16_t> cube_indices = {
            0, 1, 2,  0, 2, 3,
            4, 5, 6,  4, 6, 7,
            8, 9, 10,  8, 10, 11,
            12, 13, 14,  12, 14, 15,
            16, 17, 18,  16, 18, 19,
            20, 21, 22,  20, 22, 23
        };

        Graphics::VertexBuffer vertex_buffer(graphics_device.GetDevice(), cube_vertices);
        Graphics::IndexBuffer index_buffer(graphics_device.GetDevice(), cube_indices);
        Graphics::ConstantBuffer<TransformData> constant_buffer(graphics_device.GetDevice());

        // 5. 【Godot 風シーンツリーの構築】
        Scene::SceneTree scene_tree;
        Scene::Node* root = scene_tree.GetRootNode();

        // (1) カメラノードの追加
        auto* camera = root->AddChild(std::make_unique<Scene::Camera3D>("MainCamera"));
        camera->SetPosition(0.0f, 2.0f, -4.5f);
        camera->SetRotation(DirectX::XMConvertToRadians(20.0f), 0.0f, 0.0f); // 20度見下ろす
        camera->SetAspectRatio(static_cast<float>(window_width) / static_cast<float>(window_height));

        // (2) 自転する親ノード（親キューブ）
        auto* parent_rotator = root->AddChild(std::make_unique<RotatorNode>("ParentRotator"));
        auto* main_cube = parent_rotator->AddChild(std::make_unique<Scene::MeshInstance3D>("MainCube"));
        main_cube->SetMesh(&vertex_buffer, &index_buffer);
        main_cube->SetScale(1.0f);

        // (3) 親ノードの子として「子キューブ（衛星）」を追加！
        //     親キューブのローカル空間で右に 2.0 オフセットさせることで、
        //     親の自転に伴って子キューブが美しい円を描いて公転します
        auto* satellite_cube = parent_rotator->AddChild(std::make_unique<Scene::MeshInstance3D>("SatelliteCube"));
        satellite_cube->SetMesh(&vertex_buffer, &index_buffer);
        satellite_cube->SetPosition(2.0f, 0.0f, 0.0f); // 親から右に 2.0
        satellite_cube->SetScale(0.35f);              // 親の 35% のミニサイズ

        D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(window_width), static_cast<float>(window_height), 0.0f, 1.0f };
        D3D12_RECT scissor_rect{ 0, 0, static_cast<LONG>(window_width), static_cast<LONG>(window_height) };

        std::cout << "\n[Info] 初期化完了！Godot 風 SceneTree による階層レンダリングが開始されます。\n";
        std::cout << "[Info] 親キューブが自転し、その周りを子キューブ（衛星）が公転します！\n\n";

        auto last_time = std::chrono::high_resolution_clock::now();
        uint64_t frame_count = 0;

        // 6. メインゲームループ（Update -> Extract -> Execute 分離アーキテクチャ）
        while (window.ProcessMessages())
        {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // =========================================================================
            // フェーズ 1: 【Update（論理更新）】
            // シーンツリーを巡回し、全ノードの OnProcess(delta) を実行
            // =========================================================================
            scene_tree.Process(delta);

            // =========================================================================
            // フェーズ 2: 【Extract（描画パケット抽出）】
            // シーンツリーから描画すべきアイテム群（RenderItem）を抽出
            // Node は DirectX 12 を一切触らず、純粋なデータだけが取り出される
            // =========================================================================
            std::vector<Graphics::RenderItem> render_items = scene_tree.ExtractRenderItems();
            const DirectX::XMMATRIX view_proj = camera->GetViewMatrix() * camera->GetProjectionMatrix();

            // =========================================================================
            // フェーズ 3: 【Execute（描画実行）】
            // レンダラーが抽出されたアイテム群を GPU に描画命令として投入
            // =========================================================================
            graphics_device.BeginCommandList();
            auto* command_list = graphics_device.GetCommandList();

            ID3D12Resource* current_back_buffer = swap_chain.GetCurrentRenderTarget();
            D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = swap_chain.GetCurrentRtvHandle();
            D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = depth_buffer.GetDSVHandle();

            // リソースバリア
            D3D12_RESOURCE_BARRIER barrier_to_render{};
            barrier_to_render.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier_to_render.Transition.pResource = current_back_buffer;
            barrier_to_render.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier_to_render.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier_to_render.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            command_list->ResourceBarrier(1, &barrier_to_render);

            // クリア
            const float clear_color[4] = { 0.08f, 0.08f, 0.12f, 1.0f };
            command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
            depth_buffer.Clear(command_list, 1.0f);

            // 出力設定
            command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
            command_list->RSSetViewports(1, &viewport);
            command_list->RSSetScissorRects(1, &scissor_rect);

            // パイプラインバインド
            pipeline.Bind(command_list);

            // 抽出された描画アイテムを順次レンダリング
            for (const auto& item : render_items)
            {
                // 各ノードが計算した最新の階層ワールド行列とカメラの ViewProj 行列を合成
                DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&item.world_matrix);
                DirectX::XMMATRIX mvp = world * view_proj;
                DirectX::XMMATRIX mvp_transposed = DirectX::XMMatrixTranspose(mvp);

                TransformData transform_data{};
                DirectX::XMStoreFloat4x4(&transform_data.mvp, mvp_transposed);
                constant_buffer.Update(transform_data);

                // 定数バッファ・頂点・インデックスをバインドしてドローコール
                pipeline.SetConstantBufferView(command_list, 0, constant_buffer.GetGPUVirtualAddress());
                command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                command_list->IASetVertexBuffers(0, 1, &item.vertex_buffer->GetView());
                command_list->IASetIndexBuffer(&item.index_buffer->GetView());

                command_list->DrawIndexedInstanced(item.index_buffer->GetIndexCount(), 1, 0, 0, 0);
            }

            // リソースバリア
            D3D12_RESOURCE_BARRIER barrier_to_present{};
            barrier_to_present.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier_to_present.Transition.pResource = current_back_buffer;
            barrier_to_present.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier_to_present.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            barrier_to_present.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            command_list->ResourceBarrier(1, &barrier_to_present);

            // 提出 & 画面表示
            graphics_device.ExecuteCommandList();
            swap_chain.Present(1);
            graphics_device.WaitForGpu();

            // =========================================================================
            // フェーズ 4: 【Cleanup（遅延削除回収）】
            // フレーム中に QueueFree() されたノードを安全に回収
            // =========================================================================
            scene_tree.Cleanup();

            ++frame_count;
        }

        std::cout << "\n[Info] 描画ループを正常に終了しました。(総フレーム数: " << frame_count << ")\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n[Fatal Error] エラーが発生しました:\n" << e.what() << "\n";
        return -1;
    }

    return 0;
}
