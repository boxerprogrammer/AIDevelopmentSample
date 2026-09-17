#include "Graphics/Shader.hpp"
#include <iostream>

namespace Graphics
{
    Shader::Shader(const std::filesystem::path& file_path, std::string_view entry_point, ShaderStage stage)
    {
        // 1. ファイルの存在確認
        if (!std::filesystem::exists(file_path))
        {
            throw std::runtime_error(std::format("シェーダファイルが見つかりません: {}", file_path.string()));
        }

        // 2. コンパイル対象のシェーダモデルの選択 (シェーダモデル 5.0 を使用)
        const char* target = nullptr;
        switch (stage)
        {
        case ShaderStage::Vertex:
            target = "vs_5_0";
            break;
        case ShaderStage::Pixel:
            target = "ps_5_0";
            break;
        }

        // 3. コンパイルオプションの設定
        UINT compile_flags = 0;
#if defined(_DEBUG)
        // デバッグビルド: デバッグ情報を埋め込み、最適化を無効化してデバッグしやすくする
        compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        // リリースビルド: 最大限の最適化を適用
        compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ComPtr<ID3DBlob> error_blob;
        HRESULT hr = D3DCompileFromFile(
            file_path.c_str(),
            nullptr,                             // マクロ定義（今回はなし）
            D3D_COMPILE_STANDARD_FILE_INCLUDE,   // #include を扱えるようにする
            std::string(entry_point).c_str(),
            target,
            compile_flags,
            0,
            &blob_,
            &error_blob
        );

        // 4. エラー処理: HLSL の文法ミス等があればその内容を取り出して表示
        if (FAILED(hr))
        {
            std::string error_msg = std::format("シェーダのコンパイルに失敗しました: {} [{}]\n", file_path.string(), entry_point);
            if (error_blob)
            {
                error_msg += static_cast<const char*>(error_blob->GetBufferPointer());
            }
            throw std::runtime_error(error_msg);
        }

        std::cout << "[Graphics] シェーダを正常にコンパイルしました: " 
                  << file_path.filename().string() << " (" << entry_point << ")\n";
    }

    const void* Shader::GetBufferPointer() const noexcept
    {
        return blob_ ? blob_->GetBufferPointer() : nullptr;
    }

    size_t Shader::GetBufferSize() const noexcept
    {
        return blob_ ? blob_->GetBufferSize() : 0;
    }
}
