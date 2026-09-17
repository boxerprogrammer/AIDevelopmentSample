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

# ソースファイルの指定
set(ENGINE_SOURCES
    src/Main.cpp
    src/Core/Window.hpp
    src/Core/Window.cpp
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
    src/Scene/MeshInstance3D.hpp
    src/Scene/MeshInstance3D.cpp
)

# 実行ファイルターゲットの定義（学習・デバッグしやすいようコンソール出力を有効化）
add_executable(EduGameEngine ${ENGINE_SOURCES})

# DirectX 12 および シェーダコンパイラライブラリをリンク
target_link_libraries(EduGameEngine PRIVATE
    d3d12
    dxgi
    dxguid
    d3dcompiler
)

# ビルド後に shaders フォルダを実行ファイルの出力先ディレクトリへ自動コピー
add_custom_command(TARGET EduGameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:EduGameEngine>/shaders
)

# インクルードディレクトリの設定（src 配下をルートとしてインクルード可能にする）
target_include_directories(EduGameEngine PRIVATE
    src
)
