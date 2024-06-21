/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERTARGET_IRENDERTARGETS_H
#define LIBACCELARENDERERVK_SRC_RENDERTARGET_IRENDERTARGETS_H

#include "RenderTarget.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/RenderSettings.h>

#include <optional>
#include <string>

namespace Accela::Render
{
    class IRenderTargets
    {
        public:

            virtual ~IRenderTargets() = default;

            [[nodiscard]] virtual bool CreateRenderTarget(const RenderTargetId& renderTargetId, const std::string& tag) = 0;
            virtual void DestroyRenderTarget(const RenderTargetId& renderTargetId, bool destroyImmediately) = 0;

            [[nodiscard]] virtual std::optional<RenderTarget> GetRenderTarget(const RenderTargetId& renderTargetId) const = 0;

            [[nodiscard]] virtual bool OnRenderSettingsChanged(const RenderSettings& renderSettings) = 0;

            virtual void Destroy() = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERTARGET_IRENDERTARGETS_H
