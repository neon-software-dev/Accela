/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASKMESSAGE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASKMESSAGE_H

#include "RenderTask.h"

#include <Accela/Common/Thread/ResultMessage.h>

#include <string>
#include <memory>

namespace Accela::Render
{
    /**
     * Thread primitive for sending a message to the render thread. Contains
     * a RenderTask to be performed.
     */
    class RenderTaskMessage : public Common::ResultMessage<bool>
    {
        public:

            static constexpr const char* TYPE = "RenderTask";

            using Ptr = std::shared_ptr<RenderTaskMessage>;

        public:

            explicit RenderTaskMessage(RenderTask::Ptr task)
                : ResultMessage<bool>(TYPE)
                , m_task(std::move(task))
            { }

            [[nodiscard]] RenderTask::Ptr GetTask() const noexcept { return m_task; }

        private:

            RenderTask::Ptr m_task;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASKMESSAGE_H
