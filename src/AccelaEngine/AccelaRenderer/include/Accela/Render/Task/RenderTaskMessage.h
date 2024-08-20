/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASKMESSAGE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASKMESSAGE_H

#include "RenderTask.h"

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Thread/ResultMessage.h>

#include <string>
#include <memory>

namespace Accela::Render
{
    class ACCELA_PUBLIC RenderTaskMessageBase
    {
        public:

            virtual ~RenderTaskMessageBase() = default;

            [[nodiscard]] virtual RenderTask::Ptr GetTask() const = 0;
    };

    /**
     * Thread primitive for sending a message to the render thread. Contains
     * a RenderTask to be performed.
     */
    template <typename Ret>
    class RenderTaskMessage : public Common::ResultMessage<Ret>, public RenderTaskMessageBase
    {
        public:

            static constexpr const char* TYPE = "RenderTask";

            using Ptr = std::shared_ptr<RenderTaskMessageBase>;

        public:

            explicit RenderTaskMessage(RenderTask::Ptr task)
                : Common::ResultMessage<Ret>(TYPE)
                , m_task(std::move(task))
            { }

            [[nodiscard]] RenderTask::Ptr GetTask() const override { return m_task; }

        private:

            RenderTask::Ptr m_task;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASKMESSAGE_H
