/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_RESOURCEIDENTIFIER_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_RESOURCEIDENTIFIER_H

#include "Common.h"

#include <format>

namespace Accela::Engine
{
    class ResourceIdentifier
    {
        public:

            ResourceIdentifier() = default;

            auto operator<=>(const ResourceIdentifier&) const = default;

            [[nodiscard]] std::optional<PackageName> GetPackageName() const noexcept { return packageName; }
            [[nodiscard]] std::string GetResourceName() const noexcept { return resourceName; }
            [[nodiscard]] std::string GetUniqueName() const;

            [[nodiscard]] bool IsValid() const { return !resourceName.empty(); }
            [[nodiscard]] bool IsPackageResource() const { return packageName.has_value(); }

        protected:

            ResourceIdentifier(std::optional<PackageName> _packageName, std::string _resourceName);

        private:

            std::optional<PackageName> packageName;
            std::string resourceName;
    };

    struct PackageResourceIdentifier : public ResourceIdentifier
    {
        PackageResourceIdentifier(PackageName _packageName, std::string _resourceName)
            : ResourceIdentifier(std::move(_packageName), std::move(_resourceName))
        { }

        PackageResourceIdentifier(std::string _packageName, std::string _resourceName)
            : ResourceIdentifier(PackageName(std::move(_packageName)), std::move(_resourceName))
        { }

        explicit PackageResourceIdentifier(const ResourceIdentifier& resource)
            : ResourceIdentifier(resource)
        { }
    };

    using PRI = PackageResourceIdentifier;

    struct CustomResourceIdentifier : public ResourceIdentifier
    {
        explicit CustomResourceIdentifier(std::string _resourceName)
            : ResourceIdentifier(std::nullopt, std::move(_resourceName))
        { }

        explicit CustomResourceIdentifier(const ResourceIdentifier& resource)
            : ResourceIdentifier(resource)
        { }
    };

    using CRI = CustomResourceIdentifier;
}

template<>
struct std::hash<Accela::Engine::ResourceIdentifier>
{
    std::size_t operator()(const Accela::Engine::ResourceIdentifier& o) const noexcept {
        return std::hash<std::string>{}(o.GetUniqueName());
    }
};

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_RESOURCEIDENTIFIER_H
