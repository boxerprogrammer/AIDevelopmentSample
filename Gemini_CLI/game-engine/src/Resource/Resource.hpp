#pragma once

#include <string>
#include <string_view>
#include <memory>

namespace Resource
{
    /**
     * @brief ゲーム内で共有利用されるアセット（メッシュ、テクスチャ、マテリアル等）の共通基底クラス
     *
     * 【Godot 風 Resource 思想と Flyweight パターン】
     * 1. 独立したデータ保持:
     *    ノード（Node）はゲーム世界の「位置や親子構造」を持ちますが、
     *    リソース（Resource）は「純粋なアセットデータ（3D形状や画像データ）」だけを保持します。
     * 2. メモリ共有（Flyweight パターン）:
     *    同じ 3D モデルを画面内に 100 個配置しても、GPU メモリ（VRAM）上には 1 つだけ実体を置き、
     *    各 MeshInstance3D ノードは std::shared_ptr<Resource> を通じて同じデータを共有します。
     * 3. 自動寿命管理:
     *    すべてのノードから参照されなくなったリソースは、C++ の参照カウントにより自動的に GPU メモリから安全に解放されます。
     */
    class Resource : public std::enable_shared_from_this<Resource>
    {
    public:
        explicit Resource(std::string_view path = "")
            : resource_path_(path)
        {
        }

        virtual ~Resource() = default;

        // リソースは共有利用が前提のため、値のコピーは禁止
        Resource(const Resource&) = delete;
        Resource& operator=(const Resource&) = delete;
        Resource(Resource&&) noexcept = default;
        Resource& operator=(Resource&&) noexcept = default;

        /**
         * @brief リソースのファイルパスまたは識別名を取得
         */
        [[nodiscard]] const std::string& GetPath() const noexcept { return resource_path_; }

        /**
         * @brief リソースのファイルパスまたは識別名を設定
         */
        void SetPath(std::string_view path) { resource_path_ = path; }

    protected:
        std::string resource_path_; // リソースのパスまたは識別キー
    };
}
