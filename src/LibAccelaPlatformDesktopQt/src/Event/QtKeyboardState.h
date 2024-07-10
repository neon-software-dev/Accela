/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTKEYBOARDSTATE_H
#define LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTKEYBOARDSTATE_H

#include <Accela/Platform/Event/IKeyboardState.h>
#include <Accela/Platform/Event/SystemEvent.h>

#include <unordered_set>
#include <mutex>

namespace Accela::Platform
{
    class QtKeyboardState : public IKeyboardState
    {
        public:

            [[nodiscard]] bool IsPhysicalKeyPressed(const Platform::PhysicalKey& physicalKey) const override;
            [[nodiscard]] bool IsPhysicalKeyPressed(const Platform::ScanCode& scanCode) const override;
            [[nodiscard]] bool IsModifierPressed(const Platform::KeyMod& keyMod) const override;
            void ForceResetState() override;

            void OnGlobalEvent(const SystemEvent& systemEvent);

        private:

            mutable std::mutex m_pressedScanCodesMutex;
            std::unordered_set<ScanCode> m_pressedScanCodes;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTKEYBOARDSTATE_H
