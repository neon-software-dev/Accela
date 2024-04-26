/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMMON_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMMON_H

#include <cstdint>

namespace Accela::Engine
{
    using EntityId = std::uint32_t;

    enum class ResultWhen
    {
        /**
         * A resource is ready to be used
         */
        Ready,

        /**
         * A resource is fully loaded into the GPU
         */
        FullyLoaded
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMMON_H
