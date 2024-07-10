/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTMOUSESTATE_H
#define LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTMOUSESTATE_H

#include <Accela/Platform/Event/IMouseState.h>

namespace Accela::Platform
{
    class QtMouseState : public IMouseState
    {
        public:

            [[nodiscard]] bool IsMouseButtonPressed(const Platform::MouseButton& button) const override;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTMOUSESTATE_H
