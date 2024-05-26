#include <Accela/Engine/ResourceIdentifier.h>

namespace Accela::Engine
{

ResourceIdentifier::ResourceIdentifier(std::optional<PackageName> _packageName, std::string _resourceName)
    : packageName(std::move(_packageName))
    , resourceName(std::move(_resourceName))
{}

std::string ResourceIdentifier::GetUniqueName() const
{
    const std::string packageNameStr = packageName.has_value() ? packageName->name : std::string();
    return std::format("{}-{}", packageNameStr, resourceName);
}

}
