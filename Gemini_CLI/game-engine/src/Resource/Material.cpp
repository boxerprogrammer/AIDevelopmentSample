#include "Resource/Material.hpp"
#include "Resource/Texture2D.hpp"

namespace Resource
{
    Material::Material(std::string_view path)
        : Resource(path)
    {
    }

    std::shared_ptr<Material> Material::CreateDefault()
    {
        auto mat = std::make_shared<Material>("builtin://Material/Default");
        mat->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        mat->SetSpecularPower(32.0f);
        mat->SetSpecularIntensity(0.5f);
        return mat;
    }

    std::shared_ptr<Material> Material::CreateShiny(const DirectX::XMFLOAT4& color)
    {
        auto mat = std::make_shared<Material>("builtin://Material/Shiny");
        mat->SetColor(color);
        mat->SetSpecularPower(64.0f);
        mat->SetSpecularIntensity(0.8f);
        return mat;
    }

    std::shared_ptr<Material> Material::CreateMatte(const DirectX::XMFLOAT4& color)
    {
        auto mat = std::make_shared<Material>("builtin://Material/Matte");
        mat->SetColor(color);
        mat->SetSpecularPower(8.0f);
        mat->SetSpecularIntensity(0.1f);
        return mat;
    }
}
