/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BUILDER_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BUILDER_H

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <memory>

namespace Accela::Platform
{
    class IPlatform;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class IEngine;

    /**
     * Builder class for building an Accela::IEngine instance
     */
    class Builder
    {
        public:

            [[nodiscard]] static std::unique_ptr<IEngine> Build(
                Common::ILogger::Ptr logger,
                Common::IMetrics::Ptr metrics,
                std::shared_ptr<Platform::IPlatform> platform,
                std::shared_ptr<Render::IRenderer> renderer
            );
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BUILDER_H
