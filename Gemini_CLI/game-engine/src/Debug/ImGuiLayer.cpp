#include "Debug/ImGuiLayer.hpp"
#include "Graphics/DirectXHelper.hpp"
#include "Scene/SceneTree.hpp"
#include "Scene/Node3D.hpp"
#include "Scene/Camera3D.hpp"
#include "Scene/FlightCamera3D.hpp"
#include "Scene/DirectionalLight3D.hpp"
#include "Scene/MeshInstance3D.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/Mesh.hpp"
#include "Resource/Texture2D.hpp"
#include "Resource/Material.hpp"
#include "Core/Input.hpp"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

#include <filesystem>
#include <iostream>
#include <cmath>

// ImGui の Win32 メッセージプロシージャハンドラの前方宣言
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Debug
{
    ImGuiLayer::ImGuiLayer(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* command_queue, uint32_t buffer_count, DXGI_FORMAT rtv_format)
    {
        // 1. ImGui 専用の SRV ディスクリプタヒープを作成
        //    Dear ImGui はフォントテクスチャを 1 つの SRV (Shader Resource View) として GPU に渡すため、
        //    シェーダから参照可能なヒープ (FLAG_SHADER_VISIBLE) が最低 1 スロット必要です。
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heap_desc.NodeMask = 0;

        Graphics::ThrowIfFailed(
            device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&srv_heap_)),
            "ImGui 用 SRV ディスクリプタヒープの作成に失敗しました。"
        );

        // 2. ImGui コンテキストの初期化
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // キーボードナビゲーションを有効化

        // 3. 日本語フォントの読み込み（Windows 標準のメイリオを使用）
        //    フォントファイルが存在する場合は、日本語文字セットを登録して文字化けを防止します。
        constexpr const char* font_path = "C:\\Windows\\Fonts\\meiryo.ttc";
        if (std::filesystem::exists(font_path))
        {
            io.Fonts->AddFontFromFileTTF(font_path, 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
        }

        // 4. カラーテーマ（ダークテーマ）の適用
        ImGui::StyleColorsDark();

        // 5. Win32 バックエンドの初期化
        ImGui_ImplWin32_Init(hwnd);

        // 6. DirectX 12 バックエンドの初期化（ImGui 1.91.x 推奨の InitInfo 構造体方式）
        //    ※ フォントテクスチャの GPU アップロード時に CommandQueue->ExecuteCommandLists() が呼ばれるため、
        //       CommandQueue の指定が必須となります。
        ImGui_ImplDX12_InitInfo init_info{};
        init_info.Device = device;
        init_info.CommandQueue = command_queue;
        init_info.NumFramesInFlight = static_cast<int>(buffer_count);
        init_info.RTVFormat = rtv_format;
        init_info.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        init_info.SrvDescriptorHeap = srv_heap_.Get();
        init_info.LegacySingleSrvCpuDescriptor = srv_heap_->GetCPUDescriptorHandleForHeapStart();
        init_info.LegacySingleSrvGpuDescriptor = srv_heap_->GetGPUDescriptorHandleForHeapStart();

        if (!ImGui_ImplDX12_Init(&init_info))
        {
            throw std::runtime_error("ImGui_ImplDX12_Init に失敗しました。");
        }
    }

    ImGuiLayer::~ImGuiLayer()
    {
        // RAII: リソースの安全な破棄順序（DX12バックエンド -> Win32バックエンド -> コンテキスト）
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    std::optional<LRESULT> ImGuiLayer::HandleWin32Message(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        // ImGui がマウスやキーボードの入力を処理・消費したかどうかを判定
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param))
        {
            return 1;
        }

        // ImGui がマウスを使用している間は、ゲーム側のクリックや操作をブロックできるようにする判定フラグ
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))
        {
            return 0;
        }
        if (io.WantCaptureKeyboard && (msg >= WM_KEYFIRST && msg <= WM_KEYLAST))
        {
            return 0;
        }

        return std::nullopt;
    }

    void ImGuiLayer::BeginFrame()
    {
        // バックエンドおよび ImGui の新規フレーム処理
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::RenderDebugUI(Scene::SceneTree& scene_tree, float delta_time, size_t render_items_count)
    {
        const ImGuiIO& io = ImGui::GetIO();

        // --- 1. メインメニューバー ---
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("表示 (View)"))
            {
                ImGui::MenuItem("ImGui デモウィンドウ", nullptr, &show_demo_window_);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (show_demo_window_)
        {
            ImGui::ShowDemoWindow(&show_demo_window_);
        }

        // --- 2. パフォーマンス & エンジン情報ウィンドウ ---
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 180), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("エンジン情報 / 統計", nullptr))
        {
            ImGui::Text("フレームレート: %.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
            ImGui::Text("経過時間 (delta): %.4f 秒", delta_time);
            ImGui::Separator();
            ImGui::Text("描画アイテム数 (RenderItems): %zu", render_items_count);
            ImGui::Separator();

            if (ImGui::CollapsingHeader("入力状態 (Input Status)"))
            {
                const auto mouse_pos = Core::Input::GetMousePosition();
                const auto mouse_delta = Core::Input::GetMouseDelta();
                ImGui::Text("マウス座標: (%.0f, %.0f)", mouse_pos.x, mouse_pos.y);
                ImGui::Text("移動量(Delta): (%.1f, %.1f)", mouse_delta.x, mouse_delta.y);
                ImGui::Text("ホイール: %.1f", Core::Input::GetMouseWheelDelta());

                ImGui::Text("ボタン: L[%s] R[%s] M[%s]",
                    Core::Input::IsMouseButtonHeld(Core::MouseButton::Left) ? "ON" : "off",
                    Core::Input::IsMouseButtonHeld(Core::MouseButton::Right) ? "ON" : "off",
                    Core::Input::IsMouseButtonHeld(Core::MouseButton::Middle) ? "ON" : "off");

                ImGui::Text("WASD: W[%s] A[%s] S[%s] D[%s]",
                    Core::Input::IsKeyHeld(Core::KeyCode::W) ? "ON" : "off",
                    Core::Input::IsKeyHeld(Core::KeyCode::A) ? "ON" : "off",
                    Core::Input::IsKeyHeld(Core::KeyCode::S) ? "ON" : "off",
                    Core::Input::IsKeyHeld(Core::KeyCode::D) ? "ON" : "off");
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ヒント: 左のツリーからノードを選び、\nインスペクタでリアルタイム編集できます。");
        }
        ImGui::End();

        // --- 3. シーンヒエラルキーウィンドウ ---
        ImGui::SetNextWindowPos(ImVec2(10, 220), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("シーンヒエラルキー (Scene Hierarchy)", nullptr))
        {
            Scene::Node* root = scene_tree.GetRootNode();
            if (root)
            {
                DrawNodeHierarchy(root);
            }
            else
            {
                ImGui::TextDisabled("ルートノードが存在しません。");
            }
        }
        ImGui::End();

        // --- 4. インスペクタウィンドウ ---
        ImGui::SetNextWindowPos(ImVec2(10, 530), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 260), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("インスペクタ (Inspector)", nullptr))
        {
            DrawInspector();
        }
        ImGui::End();

        // --- 5. リソースマネージャ（Flyweight パターン監視）ウィンドウ ---
        ImGui::SetNextWindowPos(ImVec2(360, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(420, 180), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("リソースキャッシュ (Resource Monitor)", nullptr))
        {
            auto resource_infos = Resource::ResourceLoader::GetCachedResourceInfos();
            ImGui::Text("キャッシュ中リソース数: %zu", resource_infos.size());

            if (ImGui::BeginTable("ResourceTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("パス / 識別子");
                ImGui::TableSetupColumn("外部参照数 (use_count)");
                ImGui::TableHeadersRow();

                for (const auto& info : resource_infos)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", info.path.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%ld", info.use_count);
                }
                ImGui::EndTable();
            }

            if (ImGui::Button("未使用キャッシュを掃除 (Clean Unused)"))
            {
                Resource::ResourceLoader::CleanUnused();
            }
        }
        ImGui::End();
    }

    void ImGuiLayer::DrawNodeHierarchy(Scene::Node* node)
    {
        if (!node) return;

        const auto& children = node->GetChildren();
        const bool has_children = !children.empty();

        // ノード表示用フラグの設定
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (!has_children)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        else
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen; // 初期状態で展開
        }

        if (selected_node_ == node)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        // ノード種別に応じたアイコン/ラベル接頭辞
        std::string label = node->GetName();
        if (dynamic_cast<Scene::FlightCamera3D*>(node))
        {
            label = "[FlightCam] " + label;
        }
        else if (dynamic_cast<Scene::Camera3D*>(node))
        {
            label = "[Cam] " + label;
        }
        else if (dynamic_cast<Scene::DirectionalLight3D*>(node))
        {
            label = "[Light] " + label;
        }
        else if (dynamic_cast<Scene::MeshInstance3D*>(node))
        {
            label = "[Mesh] " + label;
        }
        else if (dynamic_cast<Scene::Node3D*>(node))
        {
            label = "[3D] " + label;
        }
        else
        {
            label = "[Node] " + label;
        }

        const bool is_open = ImGui::TreeNodeEx(node, flags, "%s", label.c_str());

        // クリックで選択
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            selected_node_ = node;
        }

        // 右クリックでコンテキストメニュー（遅延削除など）
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("ノードを削除 (QueueFree)"))
            {
                node->QueueFree();
                if (selected_node_ == node)
                {
                    selected_node_ = nullptr;
                }
            }
            ImGui::EndPopup();
        }

        // 子ノードの再帰的描画
        if (has_children && is_open)
        {
            for (const auto& child : children)
            {
                DrawNodeHierarchy(child.get());
            }
            ImGui::TreePop();
        }
    }

    void ImGuiLayer::DrawInspector()
    {
        if (!selected_node_)
        {
            ImGui::TextDisabled("ノードが選択されていません。");
            return;
        }

        // ノードの基本情報
        char name_buf[128] = {};
        const std::string& current_name = selected_node_->GetName();
        strncpy_s(name_buf, current_name.c_str(), sizeof(name_buf) - 1);
        if (ImGui::InputText("名前 (Name)", name_buf, sizeof(name_buf)))
        {
            selected_node_->SetName(name_buf);
        }

        ImGui::Separator();

        // 3D ノードの場合: Transform のリアルタイム編集
        auto* node_3d = dynamic_cast<Scene::Node3D*>(selected_node_);
        if (node_3d)
        {
            if (ImGui::CollapsingHeader("Transform3D (ローカル変換)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Scene::Transform3D& transform = node_3d->GetTransform();

                // 1. 位置 (Position)
                DirectX::XMFLOAT3 pos = transform.GetPosition();
                if (ImGui::DragFloat3("位置 (Position)", &pos.x, 0.05f))
                {
                    transform.SetPosition(pos);
                }

                // 2. 回転 (Rotation): ユーザーには分かりやすい度数法 (Degrees) で表示・編集
                DirectX::XMFLOAT3 rot_rad = transform.GetRotation();
                constexpr float rad_to_deg = 180.0f / 3.14159265359f;
                constexpr float deg_to_rad = 3.14159265359f / 180.0f;
                DirectX::XMFLOAT3 rot_deg = {
                    rot_rad.x * rad_to_deg,
                    rot_rad.y * rad_to_deg,
                    rot_rad.z * rad_to_deg
                };

                if (ImGui::DragFloat3("回転 (Rotation)", &rot_deg.x, 1.0f, -360.0f, 360.0f, "%.1f°"))
                {
                    transform.SetRotation(
                        rot_deg.x * deg_to_rad,
                        rot_deg.y * deg_to_rad,
                        rot_deg.z * deg_to_rad
                    );
                }

                // 3. 拡縮 (Scale)
                DirectX::XMFLOAT3 scale = transform.GetScale();
                if (ImGui::DragFloat3("拡縮 (Scale)", &scale.x, 0.05f, 0.01f, 50.0f))
                {
                    transform.SetScale(scale.x, scale.y, scale.z);
                }

                // リセットボタン
                if (ImGui::Button("トランスフォームのリセット"))
                {
                    transform.SetPosition(0.0f, 0.0f, 0.0f);
                    transform.SetRotation(0.0f, 0.0f, 0.0f);
                    transform.SetScale(1.0f, 1.0f, 1.0f);
                }
            }
        }

        // カメラノードの場合: FOV などのカメラパラメータ編集
        auto* camera = dynamic_cast<Scene::Camera3D*>(selected_node_);
        if (camera)
        {
            if (ImGui::CollapsingHeader("Camera3D 設定", ImGuiTreeNodeFlags_DefaultOpen))
            {
                float fov = camera->GetFov();
                if (ImGui::SliderFloat("視野角 (FOV)", &fov, 10.0f, 120.0f, "%.1f°"))
                {
                    camera->SetFov(fov);
                }

                float aspect = camera->GetAspectRatio();
                ImGui::Text("アスペクト比: %.2f", aspect);
            }
        }

        // フライトカメラノードの場合: 移動速度・ブースト倍率・感度設定
        auto* flight_cam = dynamic_cast<Scene::FlightCamera3D*>(selected_node_);
        if (flight_cam)
        {
            if (ImGui::CollapsingHeader("FlightCamera3D 設定", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool input_enabled = flight_cam->IsInputEnabled();
                if (ImGui::Checkbox("カメラ操作の有効化", &input_enabled))
                {
                    flight_cam->SetInputEnabled(input_enabled);
                }

                float speed = flight_cam->GetMoveSpeed();
                if (ImGui::SliderFloat("移動速度 (単位/秒)", &speed, 0.5f, 30.0f, "%.1f"))
                {
                    flight_cam->SetMoveSpeed(speed);
                }

                float boost = flight_cam->GetBoostMultiplier();
                if (ImGui::SliderFloat("Shift ブースト倍率", &boost, 1.0f, 5.0f, "%.1fx"))
                {
                    flight_cam->SetBoostMultiplier(boost);
                }

                float sensitivity = flight_cam->GetMouseSensitivity();
                if (ImGui::SliderFloat("マウス感度", &sensitivity, 0.0005f, 0.01f, "%.4f"))
                {
                    flight_cam->SetMouseSensitivity(sensitivity);
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "【操作方法】");
                ImGui::BulletText("右ボタンドラッグ: 視点回転 (Yaw/Pitch)");
                ImGui::BulletText("W / S: 前進 / 後退");
                ImGui::BulletText("A / D: 左右平行移動");
                ImGui::BulletText("E / Space: 上昇");
                ImGui::BulletText("Q / C: 下降");
                ImGui::BulletText("Left Shift: 移動速度ブースト");
                ImGui::BulletText("マウスホイール: 移動速度調整");
            }
        }

        // 平行光源ノードの場合: 光の色、強度、環境光、向きの調整
        auto* light = dynamic_cast<Scene::DirectionalLight3D*>(selected_node_);
        if (light)
        {
            if (ImGui::CollapsingHeader("DirectionalLight3D (光源設定)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // 光の色
                DirectX::XMFLOAT3 color = light->GetColor();
                if (ImGui::ColorEdit3("光の色 (Color)", &color.x))
                {
                    light->SetColor(color);
                }

                // 光の強度
                float intensity = light->GetIntensity();
                if (ImGui::SliderFloat("光の強度 (Intensity)", &intensity, 0.0f, 5.0f, "%.2f"))
                {
                    light->SetIntensity(intensity);
                }

                // 環境光の色
                DirectX::XMFLOAT3 ambient = light->GetAmbientColor();
                if (ImGui::ColorEdit3("環境光 (Ambient)", &ambient.x))
                {
                    light->SetAmbientColor(ambient);
                }

                DirectX::XMFLOAT3 dir = light->GetDirection();
                ImGui::Text("照射方向: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "【シャドウ設定 (Shadow Mapping)】");

                float bias = light->GetShadowBias();
                if (ImGui::SliderFloat("深度バイアス", &bias, 0.0001f, 0.01f, "%.4f"))
                {
                    light->SetShadowBias(bias);
                }

                float strength = light->GetShadowStrength();
                if (ImGui::SliderFloat("影の濃さ (Strength)", &strength, 0.0f, 1.0f, "%.2f"))
                {
                    light->SetShadowStrength(strength);
                }

                float box_size = light->GetShadowBoxSize();
                if (ImGui::SliderFloat("シャドウ範囲 (Box Size)", &box_size, 5.0f, 40.0f, "%.1f"))
                {
                    light->SetShadowBoxSize(box_size);
                }
            }
        }

        // メッシュインスタンスノードの場合: 共有メッシュおよびマテリアル情報の表示・編集
        auto* mesh_instance = dynamic_cast<Scene::MeshInstance3D*>(selected_node_);
        if (mesh_instance)
        {
            if (ImGui::CollapsingHeader("MeshInstance3D (メッシュ・材質情報)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto mesh = mesh_instance->GetMesh();
                if (mesh)
                {
                    ImGui::Text("共有メッシュ: %s", mesh->GetPath().c_str());
                    ImGui::Text("メッシュ参照数: %ld (共有中)", mesh.use_count());
                    ImGui::Text("インデックス数: %u", mesh->GetIndexBuffer() ? mesh->GetIndexBuffer()->GetIndexCount() : 0);
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "メッシュが設定されていません。");
                }

                ImGui::Separator();

                // マテリアルの編集
                auto material = mesh_instance->GetMaterial();
                if (material)
                {
                    ImGui::Text("マテリアル: %s", material->GetPath().c_str());
                    ImGui::Text("マテリアル参照数: %ld (共有中)", material.use_count());

                    auto tex = material->GetTexture();
                    if (tex)
                    {
                        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "テクスチャ: %s (%ux%u)",
                            tex->GetPath().c_str(), tex->GetWidth(), tex->GetHeight());
                    }
                    else
                    {
                        ImGui::TextDisabled("テクスチャ: 未設定 (デフォルト白テクスチャ)");
                    }

                    DirectX::XMFLOAT4 mat_color = material->GetColor();
                    if (ImGui::ColorEdit4("材質カラー (Albedo)", &mat_color.x))
                    {
                        material->SetColor(mat_color);
                    }

                    float spec_power = material->GetSpecularPower();
                    if (ImGui::SliderFloat("光沢の鋭さ (Power)", &spec_power, 1.0f, 128.0f, "%.1f"))
                    {
                        material->SetSpecularPower(spec_power);
                    }

                    float spec_intensity = material->GetSpecularIntensity();
                    if (ImGui::SliderFloat("反射強度 (Intensity)", &spec_intensity, 0.0f, 1.0f, "%.2f"))
                    {
                        material->SetSpecularIntensity(spec_intensity);
                    }
                }
                else
                {
                    ImGui::TextDisabled("マテリアルが設定されていません（デフォルト値を使用）");
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 削除ボタン
        if (ImGui::Button("ノードを破棄 (QueueFree)"))
        {
            selected_node_->QueueFree();
            selected_node_ = nullptr;
        }
    }

    void ImGuiLayer::Render(ID3D12GraphicsCommandList* command_list)
    {
        // 描画データの生成
        ImGui::Render();

        // ディスクリプタヒープのバインド
        // DirectX 12 では描画コマンド発行前に、シェーダが参照するヒープをコマンドリストにセットする必要があります
        ID3D12DescriptorHeap* descriptor_heaps[] = { srv_heap_.Get() };
        command_list->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);

        // ImGui のバックエンド描画実行
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list);
    }
}
