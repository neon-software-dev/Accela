/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_PACKAGE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_PACKAGE_H

#include "Manifest.h"
#include "Construct.h"

#include <Accela/Platform/Package/PackageSource.h>

#include <vector>

namespace Accela::Engine
{
    /**
     * An in-memory representation of a package
     */
    struct Package
    {
        Package() = default;

        Package(Platform::PackageSource::Ptr _source, Manifest _manifest, std::vector<Construct::Ptr> _constructs)
            : source(std::move(_source))
            , manifest(std::move(_manifest))
            , constructs(std::move(_constructs))
        { }

        /** The interface through which the package's data can be fetched */
        Platform::PackageSource::Ptr source;

        /** The package's manifest */
        Manifest manifest;

        /** The package's constructs */
        std::vector<Construct::Ptr> constructs;

        auto operator<=>(const Package&) const = default;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_PACKAGE_H
