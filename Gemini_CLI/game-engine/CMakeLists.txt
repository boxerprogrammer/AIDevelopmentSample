cmake_minimum_required(VERSION 3.20)
project(EduGameEngine LANGUAGES CXX)

# C++20 を標準として要求
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC 向けのコンパイラ設定
if (MSVC)
    # 日本語コメント対応のための UTF-8 指定と、警告レベル4
    add_compile_options(/utf-8 /W4)
endif()

# 外部ライブラリの自動取得 (Dear ImGui)
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    )

    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(imgui PUBLIC
        d3d12
        dxgi
    )
endif()

# 外部ライブラリの自動取得 (stb: 画像ファイル読み込み)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

# ソースファイルの指定
set(ENGINE_SOURCES
    src/Main.cpp
    src/Core/Window.hpp
    src/Core/Window.cpp
    src/Core/Input.hpp
    src/Core/Input.cpp
    src/Graphics/DirectXHelper.hpp
    src/Graphics/GraphicsDevice.hpp
    src/Graphics/GraphicsDevice.cpp
    src/Graphics/SwapChain.hpp
    src/Graphics/SwapChain.cpp
    src/Graphics/Shader.hpp
    src/Graphics/Shader.cpp
    src/Graphics/VertexBuffer.hpp
    src/Graphics/VertexBuffer.cpp
    src/Graphics/IndexBuffer.hpp
    src/Graphics/IndexBuffer.cpp
    src/Graphics/ConstantBuffer.hpp
    src/Graphics/DepthBuffer.hpp
    src/Graphics/DepthBuffer.cpp
    src/Graphics/Pipeline.hpp
    src/Graphics/Pipeline.cpp
    src/Graphics/ShadowMap.hpp
    src/Graphics/ShadowMap.cpp
    src/Graphics/ShadowPipeline.hpp
    src/Graphics/ShadowPipeline.cpp
    src/Graphics/RenderItem.hpp
    src/Scene/Node.hpp
    src/Scene/Node.cpp
    src/Scene/SceneTree.hpp
    src/Scene/SceneTree.cpp
    src/Scene/Transform3D.hpp
    src/Scene/Node3D.hpp
    src/Scene/Node3D.cpp
    src/Scene/Camera3D.hpp
    src/Scene/Camera3D.cpp
    src/Scene/FlightCamera3D.hpp
    src/Scene/FlightCamera3D.cpp
    src/Scene/DirectionalLight3D.hpp
    src/Scene/DirectionalLight3D.cpp
    src/Scene/MeshInstance3D.hpp
    src/Scene/MeshInstance3D.cpp
    src/Resource/Resource.hpp
    src/Resource/Mesh.hpp
    src/Resource/Mesh.cpp
    src/Resource/Texture2D.hpp
    src/Resource/Texture2D.cpp
    src/Resource/Material.hpp
    src/Resource/Material.cpp
    src/Resource/ResourceLoader.hpp
    src/Resource/ResourceLoader.cpp
    src/Debug/ImGuiLayer.hpp
    src/Debug/ImGuiLayer.cpp
)

# 実行ファイルターゲットの定義（学習・デバッグしやすいようコンソール出力を有効化）
add_executable(EduGameEngine ${ENGINE_SOURCES})

# DirectX 12, シェーダコンパイラライブラリ, ImGui をリンク
target_link_libraries(EduGameEngine PRIVATE
    d3d12
    dxgi
    dxguid
    d3dcompiler
    imgui
)

# ビルド後に shaders フォルダを実行ファイルの出力先ディレクトリへ自動コピー
add_custom_command(TARGET EduGameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:EduGameEngine>/shaders
)

# インクルードディレクトリの設定（src 配下および stb をインクルード可能にする）
target_include_directories(EduGameEngine PRIVATE
    src
    ${stb_SOURCE_DIR}
)
