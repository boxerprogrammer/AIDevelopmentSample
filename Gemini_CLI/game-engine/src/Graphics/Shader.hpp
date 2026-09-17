#pragma once

#include "Graphics/DirectXHelper.hpp"
#include <d3dcompiler.h>
#include <filesystem>
#include <string>

namespace Graphics
{
    /**
     * @brief シェーダの実行ステージ
     */
    enum class ShaderStage
    {
        Vertex, // 頂点シェーダ (vs_5_0)
        Pixel   // ピクセルシェーダ (ps_5_0)
    };

    /**
     * @brief HLSL シェーダを実行時にコンパイル・保持するクラス
     *
     * 【このクラスの責務】
     * 1. HLSL ファイルの読み込みと D3DCompileFromFile によるコンパイル
     * 2. エラー発生時に HLSL の行番号や原因を含む詳細なエラーメッセージを報告
     * 3. コンパイル結果のバイトコード (ID3DBlob) の安全な保持
     */
    class Shader
    {
    public:
        /**
         * @brief HLSL ファイルをコンパイルしてシェーダオブジェクトを生成
         * @param file_path HLSL ファイルへのパス
         * @param entry_point エントリポイント関数名（例: "VSMain", "PSMain"）
         * @param stage シェーダステージ（Vertex または Pixel）
         */
        Shader(const std::filesystem::path& file_path, std::string_view entry_point, ShaderStage stage);

        ~Shader() = default;

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) noexcept = default;
        Shader& operator=(Shader&&) noexcept = default;

        // コンパイル済みバイナリ（バイトコード）へのポインタ
        [[nodiscard]] const void* GetBufferPointer() const noexcept;

        // バイトコードのサイズ（バイト数）
        [[nodiscard]] size_t GetBufferSize() const noexcept;

        // Blob オブジェクトの生ポインタ
        [[nodiscard]] ID3DBlob* GetBlob() const noexcept { return blob_.Get(); }

    private:
        ComPtr<ID3DBlob> blob_; // コンパイル後の GPU バイトコードを保持するメモリ
    };
}
