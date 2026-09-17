#include "Resource/ResourceLoader.hpp"

namespace Resource
{
    ResourceLoader& ResourceLoader::GetInstance()
    {
        static ResourceLoader instance;
        return instance;
    }

    void ResourceLoader::Register(std::string_view path, std::shared_ptr<Resource> resource)
    {
        if (!resource) return;

        const std::string key(path);
        resource->SetPath(key);
        GetInstance().cache_[key] = resource;
    }

    bool ResourceLoader::Has(std::string_view path)
    {
        auto& cache = GetInstance().cache_;
        auto it = cache.find(std::string(path));
        if (it == cache.end()) return false;
        return !it->second.expired();
    }

    void ResourceLoader::CleanUnused()
    {
        auto& cache = GetInstance().cache_;
        for (auto it = cache.begin(); it != cache.end();)
        {
            if (it->second.expired())
            {
                it = cache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void ResourceLoader::ClearAll()
    {
        GetInstance().cache_.clear();
    }

    std::vector<ResourceInfo> ResourceLoader::GetCachedResourceInfos()
    {
        std::vector<ResourceInfo> infos;
        auto& cache = GetInstance().cache_;

        for (const auto& [path, weak_res] : cache)
        {
            if (auto locked = weak_res.lock())
            {
                // use_count() は locked による +1 が含まれるため、外部からの参照数は (use_count - 1)
                infos.push_back({ path, locked.use_count() - 1 });
            }
            else
            {
                infos.push_back({ path, 0 });
            }
        }

        return infos;
    }
}
