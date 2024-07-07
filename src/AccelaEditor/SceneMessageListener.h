/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_SCENEMESSAGELISTENER_H
#define ACCELAEDITOR_SCENEMESSAGELISTENER_H

#include <Accela/Common/Thread/Message.h>

namespace Accela
{
    struct SceneMessageListener
    {
        virtual ~SceneMessageListener() = default;

        virtual void OnSceneMessage(const Common::Message::Ptr& message) = 0;
    };
}

#endif //ACCELAEDITOR_SCENEMESSAGELISTENER_H
