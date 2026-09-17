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
#include "Graphics/ShadowMap.hpp"
#include "Graphics/ShadowPipeline.hpp"

#include "Scene/SceneTree.hpp"
#include "Scene/Node3D.hpp"
#include "Scene/Camera3D.hpp"
#include "Scene/FlightCamera3D.hpp"
#include "Scene/MeshInstance3D.hpp"
#include "Scene/DirectionalLight3D.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/Mesh.hpp"
#include "Resource/Material.hpp"
#include "Resource/Texture2D.hpp"
#include "Core/Input.hpp"
#include "Debug/ImGuiLayer.hpp"

#include <DirectXMath.h>
#include <iostream>
#include <exception>
#include <vector>
#include <chrono>

// メインシーン・光源・マテリアル・シャドウ用定数バッファデータ構造体（HLSL の SceneConstantBuffer と完全一致）
struct SceneConstantData
{
    DirectX::XMFLOAT4X4 world_matrix;            // ローカル -> ワールド変換行列 (64 bytes)
    DirectX::XMFLOAT4X4 view_proj_matrix;        // ワールド -> クリップ空間行列 (64 bytes)
    DirectX::XMFLOAT4   camera_position;         // カメラのワールド座標 (xyz) (16 bytes)
    DirectX::XMFLOAT4   light_direction;         // 平行光源の照射方向 (xyz) (16 bytes)
    DirectX::XMFLOAT4   light_color;             // 光源色 (rgb) と 光源強度 (a) (16 bytes)
    DirectX::XMFLOAT4   ambient_color;           // 環境光の色 (rgb) (16 bytes)
    DirectX::XMFLOAT4   material_color;          // マテリアル基本反射色 (rgba) (16 bytes)
    DirectX::XMFLOAT4   material_params;         // x: specular_power, y: specular_intensity (16 bytes)
    DirectX::XMFLOAT4X4 light_view_proj_matrix;  // ワールド -> ライトクリップ空間行列 (64 bytes)
    DirectX::XMFLOAT4   shadow_params;           // x: shadow_bias, y: shadow_strength, z: shadow_map_size (16 bytes)
};

// シャドウマップ生成専用定数バッファデータ構造体（HLSL の ShadowConstantBuffer と完全一致）
struct ShadowConstantData
{
    DirectX::XMFLOAT4X4 world_matrix;            // ローカル -> ワールド変換行列 (64 bytes)
    DirectX::XMFLOAT4X4 light_view_proj_matrix;  // ワールド -> ライトクリップ空間行列 (64 bytes)
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
        if (is_active_)
        {
            // 時間差分 delta を使って、フレームレートに依存しない滑らかな自転を行う
            Rotate(delta * 0.5f * speed_, delta * 1.0f * speed_, 0.0f);
        }
    }

    bool is_active_ = true;
    float speed_ = 1.0f;
};

int main()
{
    // 日本語文字化け防止
    std::setlocale(LC_ALL, ".UTF-8");

    std::cout << "========================================\n";
    std::cout << "  EduGameEngine - 教育用ゲームエンジン  \n";
    std::cout << "  Phase 7: シャドウマッピング (Shadow Mapping) \n";
    std::cout << "========================================\n\n";

    try
    {
        // 1. ウィンドウの生成
        constexpr uint32_t window_width = 1280;
        constexpr uint32_t window_height = 720;
        Core::Window window(L"EduGameEngine [DirectX 12 / C++20] - Real-time Shadow Mapping", window_width, window_height);

        // 2. DirectX 12 グラフィックス基盤
        Graphics::GraphicsDevice graphics_device(/*enable_debug_layer=*/true);
        Graphics::SwapChain swap_chain(graphics_device, window.GetHwnd(), window_width, window_height);
        Graphics::DepthBuffer depth_buffer(graphics_device.GetDevice(), window_width, window_height);

        // 3. ImGui デバッグ UI レイヤーの初期化と Window メッセージフック
        Debug::ImGuiLayer imgui_layer(
            window.GetHwnd(),
            graphics_device.GetDevice(),
            graphics_device.GetCommandQueue(),
            Graphics::SwapChain::kBufferCount,
            DXGI_FORMAT_R8G8B8A8_UNORM
        );

        // OS からのメッセージを ImGui と 入力システム (Core::Input) へ順番にルーティング
        window.SetCustomWndProcHandler([&imgui_layer](HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) -> std::optional<LRESULT> {
            // ImGui が入力（UI操作など）を消費した場合は、ゲーム側の入力には流さない（誤操作防止）
            const auto custom_result = imgui_layer.HandleWin32Message(hwnd, msg, w_param, l_param);
            if (custom_result.has_value())
            {
                return custom_result;
            }

            // ImGui が消費しなかったメッセージを入力マネージャに渡す
            Core::Input::ProcessMessage(msg, w_param, l_param);
            return std::nullopt;
        });

        // 4. メインパイプライン・シェーダの準備（ライティング・PCF ソフトシャドウ対応 Lit.hlsl）
        Graphics::Shader vertex_shader("shaders/Lit.hlsl", "VSMain", Graphics::ShaderStage::Vertex);
        Graphics::Shader pixel_shader("shaders/Lit.hlsl", "PSMain", Graphics::ShaderStage::Pixel);
        Graphics::Pipeline pipeline(graphics_device.GetDevice(), vertex_shader, pixel_shader);
        Graphics::ConstantBuffer<SceneConstantData> constant_buffer(graphics_device.GetDevice());

        // 5. シャドウマップ専用パイプライン・シェーダ・シャドウマップの準備（パス 1 用）
        Graphics::Shader shadow_vs("shaders/ShadowDepth.hlsl", "VSMain", Graphics::ShaderStage::Vertex);
        Graphics::ShadowPipeline shadow_pipeline(graphics_device.GetDevice(), shadow_vs);
        Graphics::ConstantBuffer<ShadowConstantData> shadow_constant_buffer(graphics_device.GetDevice());
        Graphics::ShadowMap shadow_map(graphics_device.GetDevice(), Graphics::ShadowMap::kDefaultResolution);

        // 6. リソース管理システム (ResourceLoader) によるメッシュとマテリアルの取得
        //    【Flyweight パターン】
        //    ResourceLoader が同一パスのリソースをキャッシュ共有し、重複生成を防ぎます。
        auto cube_mesh = Resource::ResourceLoader::GetOrLoad<Resource::Mesh>("builtin://Mesh/Cube", [&]() {
            return Resource::Mesh::CreateCube(graphics_device.GetDevice(), 1.0f);
        });

        auto plane_mesh = Resource::ResourceLoader::GetOrLoad<Resource::Mesh>("builtin://Mesh/Plane", [&]() {
            return Resource::Mesh::CreatePlane(graphics_device.GetDevice(), 14.0f, 14.0f);
        });

        // マテリアルリソースの生成
        auto cube_material = Resource::ResourceLoader::GetOrLoad<Resource::Material>("builtin://Material/Cube", []() {
            return Resource::Material::CreateShiny({ 1.0f, 1.0f, 1.0f, 1.0f });
        });

        auto satellite_material = Resource::ResourceLoader::GetOrLoad<Resource::Material>("builtin://Material/Satellite", []() {
            return Resource::Material::CreateShiny({ 1.0f, 0.85f, 0.3f, 1.0f }); // 光沢のあるゴールド
        });

        auto ground_material = Resource::ResourceLoader::GetOrLoad<Resource::Material>("builtin://Material/Ground", []() {
            return Resource::Material::CreateMatte({ 0.7f, 0.75f, 0.8f, 1.0f }); // 落ち着いたマットグレー
        });

        // テクスチャリソースの生成とキャッシュ共有 (Flyweight パターン)
        auto default_white_texture = Resource::ResourceLoader::GetOrLoad<Resource::Texture2D>("builtin://Texture/White", [&]() {
            return Resource::Texture2D::CreateWhite(graphics_device.GetDevice(), graphics_device.GetCommandQueue());
        });

        auto checkerboard_texture = Resource::ResourceLoader::GetOrLoad<Resource::Texture2D>("builtin://Texture/Checkerboard", [&]() {
            return Resource::Texture2D::CreateCheckerboard(graphics_device.GetDevice(), graphics_device.GetCommandQueue(), 256, 256, 32);
        });

        // キューブ用のプロシージャルな木箱/グリッド風テクスチャ
        auto box_texture = Resource::ResourceLoader::GetOrLoad<Resource::Texture2D>("builtin://Texture/BoxGrid", [&]() {
            constexpr uint32_t w = 128, h = 128;
            std::vector<uint32_t> pixels(w * h);
            for (uint32_t y = 0; y < h; ++y)
            {
                for (uint32_t x = 0; x < w; ++x)
                {
                    const bool is_border = (x < 5 || x >= w - 5 || y < 5 || y >= h - 5 ||
                                           std::abs(static_cast<int>(x) - static_cast<int>(y)) < 3 ||
                                           std::abs(static_cast<int>(x) + static_cast<int>(y) - static_cast<int>(w)) < 3);
                    pixels[y * w + x] = is_border ? 0xFF281E14 : 0xFFDE903A; // 濃茶枠線 + オレンジ
                }
            }
            return Resource::Texture2D::CreateFromPixels(
                graphics_device.GetDevice(),
                graphics_device.GetCommandQueue(),
                reinterpret_cast<const uint8_t*>(pixels.data()),
                w, h,
                "builtin://Texture/BoxGrid"
            );
        });

        // マテリアルへテクスチャをバインド
        cube_material->SetTexture(box_texture);
        ground_material->SetTexture(checkerboard_texture);
        // satellite_material はテクスチャ未設定（自動的に default_white_texture が使われ、ゴールド色が映える）

        // 7. 【DirectX 12 単一 CBV_SRV_UAV ヒープ共有アーキテクチャ】
        //    DirectX 12 では 1 つの描画コマンドリストに同時にバインドできる CBV_SRV_UAV ヒープは 1 つのみです。
        //    そのため、アルベドテクスチャ (t0) とシャドウマップ (t1) を同時にピクセルシェーダで参照できるよう、
        //    全リソースの SRV を 1 つのシェーダ可視ディスクリプタヒープに連続配置します。
        constexpr uint32_t kSceneSrvCount = 16;
        D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc{};
        srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_heap_desc.NumDescriptors = kSceneSrvCount;
        srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> scene_srv_heap;
        Graphics::ThrowIfFailed(
            graphics_device.GetDevice()->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&scene_srv_heap)),
            "シーン共有 SRV ディスクリプタヒープの作成に失敗しました。"
        );

        const uint32_t descriptor_size = graphics_device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = scene_srv_heap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = scene_srv_heap->GetGPUDescriptorHandleForHeapStart();

        // スロット 0: シャドウマップ SRV
        shadow_map.CreateShaderResourceViewInHeap(graphics_device.GetDevice(), cpu_handle, gpu_handle);
        cpu_handle.ptr += descriptor_size;
        gpu_handle.ptr += descriptor_size;

        // スロット 1: デフォルト白テクスチャ
        default_white_texture->CreateShaderResourceViewInHeap(graphics_device.GetDevice(), cpu_handle, gpu_handle);
        cpu_handle.ptr += descriptor_size;
        gpu_handle.ptr += descriptor_size;

        // スロット 2: チェッカーボードテクスチャ
        checkerboard_texture->CreateShaderResourceViewInHeap(graphics_device.GetDevice(), cpu_handle, gpu_handle);
        cpu_handle.ptr += descriptor_size;
        gpu_handle.ptr += descriptor_size;

        // スロット 3: 木箱グリッドテクスチャ
        box_texture->CreateShaderResourceViewInHeap(graphics_device.GetDevice(), cpu_handle, gpu_handle);

        // 5. 【Godot 風シーンツリーの構築】
        Scene::SceneTree scene_tree;
        Scene::Node* root = scene_tree.GetRootNode();

        // (1) フライトカメラノードの追加（WASD + マウス右ドラッグで自由視点移動）
        auto* camera = root->AddChild(std::make_unique<Scene::FlightCamera3D>("MainCamera"));
        camera->SetPosition(0.0f, 3.0f, -5.5f);
        camera->SetViewAngles(DirectX::XMConvertToRadians(25.0f), 0.0f); // 25度見下ろす
        camera->SetAspectRatio(static_cast<float>(window_width) / static_cast<float>(window_height));

        // (2) 平行光源ノード（太陽光）の追加！
        auto* sun_light = root->AddChild(std::make_unique<Scene::DirectionalLight3D>("SunLight"));
        sun_light->SetDirection(0.6f, -1.0f, 0.5f); // 斜め上方からの太陽光
        sun_light->SetIntensity(1.2f);
        sun_light->SetAmbientColor(0.18f, 0.18f, 0.24f);

        // (3) 地面プレーン（空間の基準となる床）
        auto* ground = root->AddChild(std::make_unique<Scene::MeshInstance3D>("GroundPlane"));
        ground->SetMesh(plane_mesh);
        ground->SetMaterial(ground_material);
        ground->SetPosition(0.0f, -1.2f, 0.0f);

        // (4) 自転する親ノード（親キューブ）
        auto* parent_rotator = root->AddChild(std::make_unique<RotatorNode>("ParentRotator"));
        auto* main_cube = parent_rotator->AddChild(std::make_unique<Scene::MeshInstance3D>("MainCube"));
        main_cube->SetMesh(cube_mesh);
        main_cube->SetMaterial(cube_material);
        main_cube->SetScale(1.0f);

        // (5) 親ノードの子として「子キューブ（衛星）」を追加！
        auto* satellite_cube = parent_rotator->AddChild(std::make_unique<Scene::MeshInstance3D>("SatelliteCube"));
        satellite_cube->SetMesh(cube_mesh);                   // 親キューブと同一の Mesh リソースを共有
        satellite_cube->SetMaterial(satellite_material);       // 固有のゴールドマテリアル
        satellite_cube->SetPosition(2.2f, 0.0f, 0.0f);        // 親から右に 2.2
        satellite_cube->SetScale(0.35f);                      // 親の 35% のミニサイズ

        // デバッグ UI の初期選択ノードをセット
        imgui_layer.SelectNode(main_cube);

        D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(window_width), static_cast<float>(window_height), 0.0f, 1.0f };
        D3D12_RECT scissor_rect{ 0, 0, static_cast<LONG>(window_width), static_cast<LONG>(window_height) };

        std::cout << "\n[Info] 初期化完了！リアルタイムシャドウマッピング (2 パス描画 + 3x3 PCF) が稼働しました。\n";
        std::cout << "[Info] 地面プレーンに親キューブおよび衛星キューブの動的な影がリアルタイム投影されます。\n";
        std::cout << "[Info] ImGui の SunLight インスペクタからシャドウバイアス・影の濃さ・範囲をリアルタイム調整可能です。\n";
        std::cout << "[Info] 操作方法:\n";
        std::cout << "       - マウス右ドラッグ: 視線回転 (Yaw / Pitch)\n";
        std::cout << "       - W / A / S / D: 前後左右の平行移動\n";
        std::cout << "       - E / Space: 上昇,  Q / C: 下降\n";
        std::cout << "       - Left Shift: 高速ブースト移動,  マウスホイール: 移動速度調整\n\n";

        auto last_time = std::chrono::high_resolution_clock::now();
        uint64_t frame_count = 0;

        // 6. メインゲームループ（Update -> Extract -> Execute 分離アーキテクチャ）
        while (true)
        {
            // 入力フレームの初期化（前フレーム状態の保持・移動量デルタのリセット）
            Core::Input::NewFrame();

            // OS からのメッセージを処理（ここで Input::ProcessMessage が呼ばれ、今フレームの移動量が蓄積される）
            if (!window.ProcessMessages())
            {
                break;
            }

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

            // ImGui デバッグ UI のフレーム開始と UI 構築
            imgui_layer.BeginFrame();
            imgui_layer.RenderDebugUI(scene_tree, delta, render_items.size());

            // =========================================================================
            // フェーズ 3: 【Execute（描画実行）- 2 パスレンダリング】
            // パス 1: 光源視点の深度書き込み（Shadow Depth Pass）
            // パス 2: メインカメラ視点のカラー＋陰影描画（Main Lit Pass with Soft Shadow）
            // =========================================================================
            graphics_device.BeginCommandList();
            auto* command_list = graphics_device.GetCommandList();

            // 光源視点の View-Projection 行列を計算
            const DirectX::XMMATRIX light_view_proj = sun_light->GetLightViewProjectionMatrix();
            const DirectX::XMMATRIX light_view_proj_transposed = DirectX::XMMatrixTranspose(light_view_proj);

            // -------------------------------------------------------------------------
            // パス 1: 【シャドウマップ生成パス (Shadow Depth Pass)】
            // -------------------------------------------------------------------------
            // 1. シャドウテクスチャを深度書き込み状態へ遷移
            shadow_map.TransitionToDepthWrite(command_list);
            shadow_map.Clear(command_list);

            // 2. レンダーターゲット（DSV のみ、カラーなし）、ビューポート、シザー矩形を設定
            D3D12_CPU_DESCRIPTOR_HANDLE shadow_dsv = shadow_map.GetDSVHandle();
            command_list->OMSetRenderTargets(0, nullptr, FALSE, &shadow_dsv);
            command_list->RSSetViewports(1, &shadow_map.GetViewport());
            command_list->RSSetScissorRects(1, &shadow_map.GetScissorRect());

            // 3. シャドウ専用パイプラインをバインド
            shadow_pipeline.Bind(command_list);

            // 4. 全メッシュをライト視点で描画し、深度値のみを高速に記録
            for (const auto& item : render_items)
            {
                DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&item.world_matrix);
                DirectX::XMMATRIX world_transposed = DirectX::XMMatrixTranspose(world);

                ShadowConstantData shadow_cb{};
                DirectX::XMStoreFloat4x4(&shadow_cb.world_matrix, world_transposed);
                DirectX::XMStoreFloat4x4(&shadow_cb.light_view_proj_matrix, light_view_proj_transposed);
                shadow_constant_buffer.Update(shadow_cb);

                shadow_pipeline.SetConstantBufferView(command_list, 0, shadow_constant_buffer.GetGPUVirtualAddress());
                command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                command_list->IASetVertexBuffers(0, 1, &item.vertex_buffer->GetView());
                command_list->IASetIndexBuffer(&item.index_buffer->GetView());
                command_list->DrawIndexedInstanced(item.index_buffer->GetIndexCount(), 1, 0, 0, 0);
            }

            // 5. シャドウテクスチャをピクセルシェーダ読み取り状態へ遷移
            shadow_map.TransitionToShaderResource(command_list);

            // -------------------------------------------------------------------------
            // パス 2: 【メインカラー描画パス (Main Lit Color Pass)】
            // -------------------------------------------------------------------------
            ID3D12Resource* current_back_buffer = swap_chain.GetCurrentRenderTarget();
            D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = swap_chain.GetCurrentRtvHandle();
            D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = depth_buffer.GetDSVHandle();

            // バックバッファを RENDER_TARGET へ遷移
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

            // レンダーターゲット設定
            command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
            command_list->RSSetViewports(1, &viewport);
            command_list->RSSetScissorRects(1, &scissor_rect);

            // メインパイプラインをバインド
            pipeline.Bind(command_list);

            // シーン共有 SRV ディスクリプタヒープをバインド
            ID3D12DescriptorHeap* descriptor_heaps[] = { scene_srv_heap.Get() };
            command_list->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);

            // ルートパラメータ 2 にシャドウマップ SRV をバインド（全メッシュ共通）
            pipeline.SetDescriptorTable(command_list, 2, shadow_map.GetSRVGpuHandle());

            // 抽出された描画アイテムを順次レンダリング
            const DirectX::XMMATRIX view_proj_transposed = DirectX::XMMatrixTranspose(view_proj);
            const DirectX::XMFLOAT3 cam_pos = camera->GetTransform().GetPosition();
            const DirectX::XMFLOAT3 light_dir = sun_light->GetDirection();
            const DirectX::XMFLOAT3 light_col = sun_light->GetColor();
            const DirectX::XMFLOAT3 ambient_col = sun_light->GetAmbientColor();

            for (const auto& item : render_items)
            {
                // ワールド行列の転置
                DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&item.world_matrix);
                DirectX::XMMATRIX world_transposed = DirectX::XMMatrixTranspose(world);

                // シェーダに渡す定数バッファデータを構築
                SceneConstantData cb_data{};
                DirectX::XMStoreFloat4x4(&cb_data.world_matrix, world_transposed);
                DirectX::XMStoreFloat4x4(&cb_data.view_proj_matrix, view_proj_transposed);
                cb_data.camera_position = { cam_pos.x, cam_pos.y, cam_pos.z, 1.0f };
                cb_data.light_direction = { light_dir.x, light_dir.y, light_dir.z, 0.0f };
                cb_data.light_color = { light_col.x, light_col.y, light_col.z, sun_light->GetIntensity() };
                cb_data.ambient_color = { ambient_col.x, ambient_col.y, ambient_col.z, 1.0f };

                // マテリアルパラメータ（設定されていない場合は標準値）
                if (item.material)
                {
                    cb_data.material_color = item.material->GetColor();
                    cb_data.material_params = {
                        item.material->GetSpecularPower(),
                        item.material->GetSpecularIntensity(),
                        0.0f,
                        0.0f
                    };
                }
                else
                {
                    cb_data.material_color = { 1.0f, 1.0f, 1.0f, 1.0f };
                    cb_data.material_params = { 32.0f, 0.5f, 0.0f, 0.0f };
                }

                // シャドウパラメータ
                DirectX::XMStoreFloat4x4(&cb_data.light_view_proj_matrix, light_view_proj_transposed);
                cb_data.shadow_params = {
                    sun_light->GetShadowBias(),
                    sun_light->GetShadowStrength(),
                    static_cast<float>(shadow_map.GetResolution()),
                    0.0f
                };

                constant_buffer.Update(cb_data);

                // マテリアルのテクスチャを取得（未設定ならデフォルト白テクスチャを使用）
                std::shared_ptr<Resource::Texture2D> active_texture = default_white_texture;
                if (item.material && item.material->GetTexture())
                {
                    active_texture = item.material->GetTexture();
                }

                // 定数バッファ (b0) & アルベドテクスチャ (t0) をバインド
                pipeline.SetConstantBufferView(command_list, 0, constant_buffer.GetGPUVirtualAddress());
                pipeline.SetDescriptorTable(command_list, 1, active_texture->GetGpuDescriptorHandle());

                command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                command_list->IASetVertexBuffers(0, 1, &item.vertex_buffer->GetView());
                command_list->IASetIndexBuffer(&item.index_buffer->GetView());

                command_list->DrawIndexedInstanced(item.index_buffer->GetIndexCount(), 1, 0, 0, 0);
            }

            // ImGui デバッグ UI の描画（3D シーンの手前にオーバーレイ）
            imgui_layer.Render(command_list);

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
