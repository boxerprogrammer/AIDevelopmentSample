#pragma once

#include "Resource/Resource.hpp"
#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

namespace Resource
{
    /**
     * @brief キャッシュ中のリソース情報（デバッグ・モニタリング用）
     */
    struct ResourceInfo
    {
        std::string path;
        long use_count; // 現在の shared_ptr 参照数
    };

    /**
     * @brief リソースのロードとキャッシュ共有を統括するマネージャクラス（Flyweight パターンの実現）
     *
     * 【std::weak_ptr キャッシュによる自動共有・自動解放】
     * 1. 既存リソースの共有:
     *    すでに誰かがそのパスのリソース (std::shared_ptr) を保持している場合、
     *    weak_ptr.lock() で既存インスタンスを返します（重複メモリ生成を防止）。
     * 2. 自動寿命管理:
     *    すべてのノードがリソースを手放すと、参照カウントが 0 になり GPU メモリから自動解放されます。
     *    weak_ptr は期限切れ (expired) になるため、ダングリングポインタの心配もありません。
     */
    class ResourceLoader
    {
    public:
        /**
         * @brief リソースを明示的にキャッシュに登録する（ビルトイン形状など）
         */
        static void Register(std::string_view path, std::shared_ptr<Resource> resource);

        /**
         * @brief キャッシュから取得、無ければローダー関数で生成してキャッシュ登録
         * @tparam T Resource を継承したクラス型
         * @param path リソースの識別パス
         * @param loader キャッシュ未ヒット時に呼ばれる生成・ロード関数
         */
        template <typename T>
        static std::shared_ptr<T> GetOrLoad(std::string_view path, std::function<std::shared_ptr<T>()> loader)
        {
            return GetInstance().GetOrLoadInternal<T>(path, loader);
        }

        /**
         * @brief 指定したパスのリソースがキャッシュ内に存在し、現在有効か確認
         */
        static bool Has(std::string_view path);

        /**
         * @brief 誰からも参照されなくなった期限切れ (expired) のキャッシュエントリを掃除
         */
        static void CleanUnused();

        /**
         * @brief すべてのキャッシュをクリア
         */
        static void ClearAll();

        /**
         * @brief 現在キャッシュされているリソースの情報一覧を取得（デバッグ UI 表示用）
         */
        static std::vector<ResourceInfo> GetCachedResourceInfos();

    private:
        ResourceLoader() = default;
        ~ResourceLoader() = default;

        static ResourceLoader& GetInstance();

        template <typename T>
        std::shared_ptr<T> GetOrLoadInternal(std::string_view path, std::function<std::shared_ptr<T>()> loader)
        {
            const std::string key(path);
            auto it = cache_.find(key);
            if (it != cache_.end())
            {
                if (auto locked = it->second.lock())
                {
                    // キャッシュヒット！既存のインスタンスを安全に共有
                    return std::dynamic_pointer_cast<T>(locked);
                }
            }

            // キャッシュが存在しない、または全員が解放して破棄済みの場合は新規生成
            std::shared_ptr<T> new_resource = loader();
            if (new_resource)
            {
                new_resource->SetPath(key);
                cache_[key] = new_resource;
            }
            return new_resource;
        }

    private:
        std::unordered_map<std::string, std::weak_ptr<Resource>> cache_;
    };
}
