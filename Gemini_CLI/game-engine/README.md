# EduGameEngine - 教育用 DirectX 12 / C++20 ゲームエンジン

過度な最適化や効率化よりも「コードの読みやすさ」「オブジェクト指向としての責務分離」「構造の理解しやすさ」を最優先した、中高生・初学者向けの教育用自作ゲームエンジンです。
アーキテクチャは **Godot エンジン（Node / Scene / Resource 構造）** を手本としています。

📖 **詳細な設計意図・アーキテクチャ解説**: [docs/WORK_PROGRESS_AND_DESIGN_INTENT.md](docs/WORK_PROGRESS_AND_DESIGN_INTENT.md)

---

## 🛠 技術スタック
- **言語規格**: C++20 (MSVC / Windows SDK 10.0.26100.0)
- **グラフィックス API**: DirectX 12 (Direct3D 12, DXGI, HLSL, DirectXMath)
- **ビルドシステム**: CMake 3.20+
- **アーキテクチャパターン**: Composite パターン, Flyweight パターン, Update-Extract-Execute 描画分離

---

## 🚀 ビルド & 実行方法

### 1. CMake によるプロジェクト構成
```pwsh
cmake -B build
```

### 2. ビルド
```pwsh
cmake --build build --config Debug
```

### 3. 実行
```pwsh
./build/Debug/EduGameEngine.exe
```

---

## 📊 現在の進捗状況 (Progress)

### ✅ Phase 1: レンダリング基盤（DirectX 12 最小描画〜3D キューブ）
- [x] **Win32 ウィンドウ & メインループ** ([src/Core/Window.hpp](src/Core/Window.hpp))
  - RAII によるウィンドウライフサイクル管理、`GWLP_USERDATA` による `this` 紐付け、`PeekMessage` ポーリング
- [x] **DirectX 12 コアデバイス** ([src/Graphics/GraphicsDevice.hpp](src/Graphics/GraphicsDevice.hpp))
  - `ID3D12Device`, `ID3D12CommandQueue`, `ID3D12CommandAllocator`, `ID3D12GraphicsCommandList`
  - `ID3D12Fence` による CPU-GPU 同期
- [x] **スワップチェイン & RTV** ([src/Graphics/SwapChain.hpp](src/Graphics/SwapChain.hpp))
  - ダブルバッファリング (`kBufferCount = 2`), RTV ディスクリプタヒープ, `Present(1)` 垂直同期
- [x] **リソースバリア & 画面クリア**
  - `D3D12_RESOURCE_STATE_PRESENT` $\leftrightarrow$ `RENDER_TARGET` の状態遷移
- [x] **シェーダ実行時コンパイラ** ([src/Graphics/Shader.hpp](src/Graphics/Shader.hpp))
  - `D3DCompileFromFile` による HLSL 読み込みと行番号付きエラーハンドリング
- [x] **頂点 & インデックスバッファ** ([src/Graphics/VertexBuffer.hpp](src/Graphics/VertexBuffer.hpp), [src/Graphics/IndexBuffer.hpp](src/Graphics/IndexBuffer.hpp))
  - Upload Heap, 16bit インデックス, 頂点キャッシュ最適化
- [x] **定数バッファ (CBV)** ([src/Graphics/ConstantBuffer.hpp](src/Graphics/ConstantBuffer.hpp))
  - 256 バイト境界アライメント, 永続マップ (Persistent Mapping), ルート記述子方式
- [x] **深度バッファ (Zバッファ / DSV)** ([src/Graphics/DepthBuffer.hpp](src/Graphics/DepthBuffer.hpp))
  - Default Heap (VRAM 専用メモリ), 32bit float 深度テクスチャ, Z テスト
- [x] **3D キューブ描画** ([shaders/SimpleTriangle.hlsl](shaders/SimpleTriangle.hlsl))
  - 背面カリング (`CULL_MODE_BACK`), 24 頂点・36 インデックスのサイコロ型カラーキューブ

### ✅ Phase 2: Godot 風シーン・ノード構造（Scene & Node System）
- [x] **基底 Node クラス** ([src/Scene/Node.hpp](src/Scene/Node.hpp))
  - Composite パターン, `std::unique_ptr<Node>`（親 $\rightarrow$ 子）と生ポインタ `Node*`（子 $\rightarrow$ 親）
  - ボトムアップ `OnReady()`, 統一ライフサイクル (`OnEnterTree`, `OnProcess`, `OnExitTree`)
  - 安全な遅延削除 `QueueFree()`
- [x] **シーンツリー** ([src/Scene/SceneTree.hpp](src/Scene/SceneTree.hpp))
  - ルートノード統括, `Process(delta)` 伝播, フレーム末尾の遅延削除回収 `Cleanup()`
- [x] **階層トランスフォーム** ([src/Scene/Transform3D.hpp](src/Scene/Transform3D.hpp), [src/Scene/Node3D.hpp](src/Scene/Node3D.hpp))
  - $S \times R \times T$ ローカル行列, 親のワールド行列を再帰的に掛け合わせるシーングラフ
- [x] **カメラノード** ([src/Scene/Camera3D.hpp](src/Scene/Camera3D.hpp))
  - ワールド行列の逆行列による View 行列, 透視投影 Projection 行列
- [x] **描画分離アーキテクチャ** ([src/Graphics/RenderItem.hpp](src/Graphics/RenderItem.hpp), [src/Scene/MeshInstance3D.hpp](src/Scene/MeshInstance3D.hpp))
  - `Update`（論理更新） $\rightarrow$ `Extract`（描画パケット抽出） $\rightarrow$ `Execute`（描画実行） $\rightarrow$ `Cleanup`（安全削除）
- [x] **サンプルシーン** ([src/Main.cpp](src/Main.cpp))
  - 自転する親キューブ (`RotatorNode`) と、その周りを公転する子キューブ (`SatelliteCube`)

### ✅ Phase 2+: Dear ImGui デバッグ UI の統合（Debug UI）
- [x] **CMake FetchContent 自動取得** ([CMakeLists.txt](CMakeLists.txt))
  - Dear ImGui (v1.91.8) の Win32 + DirectX 12 バックエンドをビルドシステムに統合
- [x] **RAII ImGui 管理レイヤー** ([src/Debug/ImGuiLayer.hpp](src/Debug/ImGuiLayer.hpp), [src/Debug/ImGuiLayer.cpp](src/Debug/ImGuiLayer.cpp))
  - 専用 SRV ディスクリプタヒープ管理、日本語メイリオフォント自動ロード、ダークテーマ
  - `Window` クラスのメッセージフック（`CustomWndProcHandler`）による入力制御
- [x] **シーンヒエラルキー (Scene Hierarchy)**
  - シーンツリーをリアルタイム階層表示、ノードの選択、右クリックによる `QueueFree()` 実行
- [x] **インスペクタ (Inspector)**
  - 選択ノードの `Transform3D`（位置・回転度数法・拡縮）のリアルタイムスライダー編集、リセット機能
  - `Camera3D` の FOV（視野角）リアルタイム編集
  - `MeshInstance3D` のバッファ状態確認
- [x] **エンジン統計・パフォーマンス表示**
  - FPS、フレーム時間 (ms)、描画 `RenderItem` 数のリアルタイムモニタリング

### ✅ Phase 3: リソース管理システム（Resource & ResourceLoader）
- [x] **Resource 基底クラス** ([src/Resource/Resource.hpp](src/Resource/Resource.hpp))
  - Godot 風 Resource 思想、`std::enable_shared_from_this`、パス識別
- [x] **Mesh リソース** ([src/Resource/Mesh.hpp](src/Resource/Mesh.hpp), [src/Resource/Mesh.cpp](src/Resource/Mesh.cpp))
  - 頂点・インデックスバッファの所有とカプセル化、キューブおよびプレーン（床面）生成ファクトリ
- [x] **ResourceLoader (Flyweight パターン)** ([src/Resource/ResourceLoader.hpp](src/Resource/ResourceLoader.hpp), [src/Resource/ResourceLoader.cpp](src/Resource/ResourceLoader.cpp))
  - `std::weak_ptr` キャッシュによる同一アセットの自動共有と完全自動解放（VRAM 浪費防止）
  - デバッグ監視用リソース情報収集（パス・参照カウント）
- [x] **MeshInstance3D リファクタリング** ([src/Scene/MeshInstance3D.hpp](src/Scene/MeshInstance3D.hpp))
  - 生バッファポインタを廃止し、`std::shared_ptr<Resource::Mesh>` を保持するクリーンな設計へ移行
- [x] **ImGui リソースモニター統合** ([src/Debug/ImGuiLayer.cpp](src/Debug/ImGuiLayer.cpp))
  - キャッシュ中のリソース一覧、外部参照数（`use_count`）のリアルタイム監視、未使用キャッシュ掃除ボタン

### ✅ Phase 4: マテリアル & ライティング機能（Lighting & Material）
- [x] **頂点法線・UV フォーマット拡張** ([src/Graphics/VertexBuffer.hpp](src/Graphics/VertexBuffer.hpp), [src/Graphics/Pipeline.cpp](src/Graphics/Pipeline.cpp))
  - `Vertex` 構造体に法線 `normal` (Nx, Ny, Nz) とテクスチャ座標 `texcoord` (U, V) を追加
  - パイプライン入力レイアウト (InputLayout) を 48 バイトアライメントで整合
- [x] **ライティング対応 HLSL シェーダ** ([shaders/Lit.hlsl](shaders/Lit.hlsl))
  - 環境光 (Ambient)、ランバート拡散反射 (Lambert Diffuse: $\cos\theta$)、ブリン・フォン鏡面反射 (Blinn-Phong Specular: ハーフベクトル) の実装
- [x] **Material リソース** ([src/Resource/Material.hpp](src/Resource/Material.hpp), [src/Resource/Material.cpp](src/Resource/Material.cpp))
  - 表面材質色（Albedo）、光沢の鋭さ（Specular Power）、反射強度（Specular Intensity）のプロパティ化
  - Shiny（光沢）、Matte（粘土・布風）等のファクトリ提供と ResourceLoader キャッシュ共有
- [x] **平行光源ノード (DirectionalLight3D)** ([src/Scene/DirectionalLight3D.hpp](src/Scene/DirectionalLight3D.hpp), [src/Scene/DirectionalLight3D.cpp](src/Scene/DirectionalLight3D.cpp))
  - ノードの回転姿勢から光線方向を自動算出、光源色・強度・環境光パラメータの保持
- [x] **ImGui 光源・材質インスペクタ統合** ([src/Debug/ImGuiLayer.cpp](src/Debug/ImGuiLayer.cpp))
  - 太陽光の向き・色・強度・環境光をスライダーでリアルタイム編集
  - 選択中メッシュのマテリアル色（カラーピッカー）や光沢パラメータのリアルタイム調整

---

### ✅ Phase 5: 入力管理システム & フライトカメラ（Input & FlightCamera）
- [x] **入力抽象化システム (Input)** ([src/Core/Input.hpp](src/Core/Input.hpp), [src/Core/Input.cpp](src/Core/Input.cpp))
  - Win32 メッセージとゲームロジックの疎結合化（OS依存を隠蔽）
  - キー＆マウスの3態判定（`IsKeyHeld` / `IsKeyPressed` / `IsKeyReleased`）
  - マウス移動量（デルタ `dx, dy`）およびホイール回転量のフレーム間集約
  - Godot 風の便利ヘルパー（`GetAxis`, `GetVector`）による正規化 2D 入力ベクトル取得
  - フォーカス喪失時（`WM_ACTIVATE` / `WM_KILLFOCUS`）の安全な入力自動リセット
- [x] **トランスフォーム方向ベクトル算出** ([src/Scene/Transform3D.hpp](src/Scene/Transform3D.hpp), [src/Scene/Node3D.hpp](src/Scene/Node3D.hpp))
  - 回転行列から `GetForward()` (+Z), `GetRight()` (+X), `GetUp()` (+Y) を算出
  - 親ノードの回転も合成したワールド空間方向ベクトルの提供
- [x] **フライトカメラノード (FlightCamera3D)** ([src/Scene/FlightCamera3D.hpp](src/Scene/FlightCamera3D.hpp), [src/Scene/FlightCamera3D.cpp](src/Scene/FlightCamera3D.cpp))
  - `Camera3D` を継承し、`OnProcess(delta)` 内で直感的な自由移動・旋回を実現
  - マウス右ボタンドラッグによる視線回転（Pitch: -89°〜+89° ジンバルロックガード, Yaw: 左右旋回）
  - WASD（前後左右移動）、E/Space（上昇）、Q/C（下降）
  - Left Shift 押下による移動速度ブースト、マウスホイールによる移動速度の動的調整
- [x] **ImGui との入力調停 & デバッグ UI 統合** ([src/Debug/ImGuiLayer.cpp](src/Debug/ImGuiLayer.cpp), [src/Main.cpp](src/Main.cpp))
  - ImGui が UI 操作でキャプチャした入力メッセージはゲーム側に流さない調停フック
  - パフォーマンスウィンドウでのマウス座標・移動量・押下状態のリアルタイムモニタリング
  - インスペクタでのフライトカメラ移動速度・ブースト倍率・感度のスライダー調整

---

### ✅ Phase 6: テクスチャマッピング機能（Texture & Sampler）
- [x] **stb 統合 (FetchContent)** ([CMakeLists.txt](CMakeLists.txt))
  - `stb_image.h` による PNG / JPG / BMP 等の画像ファイル読み込み環境の構築
- [x] **2D テクスチャリソース (Texture2D)** ([src/Resource/Texture2D.hpp](src/Resource/Texture2D.hpp), [src/Resource/Texture2D.cpp](src/Resource/Texture2D.cpp))
  - GPU 専用メモリ（Default Heap）テクスチャリソースの排他所有（RAII）
  - 中間アップロードバッファ（Upload Heap）と 256 バイト行ピッチアライメント (`GetCopyableFootprints`)
  - `CopyTextureRegion` による VRAM 転送とリソースバリア（`COPY_DEST` $\rightarrow$ `PIXEL_SHADER_RESOURCE`）
  - 一回限りコマンドの GPU 実行および Fence による CPU-GPU 同期待機
  - 各テクスチャ自律のシェーダ可視 SRV ディスクリプタヒープ管理
  - プロシージャル生成ファクトリ（デフォルト 1x1 白テクスチャ、市松模様チェッカーボード、木箱風枠線テクスチャ）
- [x] **パイプライン & シェーダ統合** ([src/Graphics/Pipeline.cpp](src/Graphics/Pipeline.cpp), [shaders/Lit.hlsl](shaders/Lit.hlsl))
  - ルートパラメータ 1 に SRV ディスクリプタテーブル (`register(t0)`) を追加
  - ルートシグネチャにバイリニアフィルタ・Wrap モードのスタティックサンプラー (`register(s0)`) を統合
  - `Lit.hlsl` 内でテクスチャサンプリングを実行し、頂点カラー・マテリアル反射色と乗算
  - テクスチャ未設定マテリアルには白テクスチャを自動バインドし、シェーダ分岐（if文）を完全排除
- [x] **マテリアル & デバッグ UI 統合** ([src/Resource/Material.hpp](src/Resource/Material.hpp), [src/Debug/ImGuiLayer.cpp](src/Debug/ImGuiLayer.cpp), [src/Main.cpp](src/Main.cpp))
  - `Material` リソースへの `Texture2D` 保持と ResourceLoader による Flyweight キャッシュ共有
  - 地面プレーンへのチェッカーボードテクスチャ、自転キューブへの木箱テクスチャ適用
  - ImGui インスペクタでのテクスチャパスおよび解像度（幅・高さ）表示

---

### ✅ Phase 7: シャドウマッピング機能（Shadow Mapping & 3x3 PCF）
- [x] **Typeless 深度テクスチャ & ビュー管理 (ShadowMap)** ([src/Graphics/ShadowMap.hpp](src/Graphics/ShadowMap.hpp), [src/Graphics/ShadowMap.cpp](src/Graphics/ShadowMap.cpp))
  - `DXGI_FORMAT_R32_TYPELESS` テクスチャ生成（DSV書き込みとSRV読み取りの両立）
  - 2048 x 2048 高解像度シャドウマップ、正方形ビューポート・シザー矩形管理
  - リソースバリア（`DEPTH_WRITE` $\leftrightarrow$ `PIXEL_SHADER_RESOURCE`）の安全な状態遷移
- [x] **深度専用シェーダ & パイプライン (ShadowPipeline)** ([shaders/ShadowDepth.hlsl](shaders/ShadowDepth.hlsl), [src/Graphics/ShadowPipeline.hpp](src/Graphics/ShadowPipeline.hpp), [src/Graphics/ShadowPipeline.cpp](src/Graphics/ShadowPipeline.cpp))
  - 頂点シェーダのみでピクセルシェーダ不使用、カラーレンダーターゲット 0 個 (`NumRenderTargets = 0`) による最速深度記録
  - ハードウェアスロープスケーリング深度バイアス (`DepthBias = 100`, `SlopeScaledDepthBias = 1.5f`) によるシャドウアクネ防止
- [x] **DirectionalLight3D の拡張** ([src/Scene/DirectionalLight3D.hpp](src/Scene/DirectionalLight3D.hpp), [src/Scene/DirectionalLight3D.cpp](src/Scene/DirectionalLight3D.cpp))
  - 直交投影 (`XMMatrixOrthographicLH`) による光源視点 View-Projection 行列算出
  - シャドウバイアス、影の濃さ（強度）、シャドウ範囲（Box Size）のプロパティ化
- [x] **ハードウェア比較サンプラー & 3x3 PCF フィルタ** ([src/Graphics/Pipeline.cpp](src/Graphics/Pipeline.cpp), [shaders/Lit.hlsl](shaders/Lit.hlsl))
  - ルートシグネチャにハードウェア深度比較サンプラー (`register(s1)`: Border White, `LESS_EQUAL`) を統合
  - HLSL の `SampleCmpLevelZero` と $3 \times 3$ カーネルによる滑らかな半影（ソフトシャドウ）生成
  - 直接光（拡散＋鏡面反射）にのみ影を適用し、環境光 (ambient) を保つ物理的に自然な減衰
- [x] **DirectX 12 単一共有 SRV ディスクリプタヒープ** ([src/Main.cpp](src/Main.cpp), [src/Resource/Texture2D.hpp](src/Resource/Texture2D.hpp))
  - 1 コマンドリストでバインド可能な CBV_SRV_UAV ヒープは 1 つという制限を解決
  - シャドウマップ (`t1`) と各マテリアルのアルベドテクスチャ (`t0`) を同一ヒープに配置して同時参照
- [x] **2 パスレンダリング統合 & ImGui 調整** ([src/Main.cpp](src/Main.cpp), [src/Debug/ImGuiLayer.cpp](src/Debug/ImGuiLayer.cpp))
  - パス 1: 光源視点の深度書き込み $\rightarrow$ パス 2: メインカラー描画
  - ImGui の SunLight インスペクタからシャドウバイアス・影の濃さ・範囲をリアルタイム操作

---

## 🧭 次回着手タスク候補

1. **【最優先候補】モデルローダーの実装（OBJ / glTF 形式 3D モデル読み込み）**
   - tinyobjloader または cgltf による外部 3D メッシュファイルのロード
   - 複雑なメッシュ（キャラクター、建物、乗り物など）の描画
2. **【次点候補】オーディオシステムの実装（XAudio2）**
   - BGM や効果音（SE）の再生・3D ポジショナルオーディオ
3. **【拡張候補】スクリプティングシステム（Lua + sol2）**
   - C++ をリコンパイルせずにノードの挙動をホットリロード記述



