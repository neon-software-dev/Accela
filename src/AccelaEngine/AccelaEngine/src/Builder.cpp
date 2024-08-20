/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Builder.h>

#include "Engine.h"

namespace Accela::Engine
{

std::unique_ptr<IEngine> Builder::Build(
    Common::ILogger::Ptr logger,
    Common::IMetrics::Ptr metrics,
    std::shared_ptr<Platform::IPlatform> platform,
    std::shared_ptr<Render::IRenderer> renderer)
{
    return std::make_unique<Engine>(
        std::move(logger),
        std::move(metrics),
        std::move(platform),
        std::move(renderer)
    );
}

}
