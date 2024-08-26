/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PLATFORMDESKTOP_H
#define LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PLATFORMDESKTOP_H

#include <Accela/Platform/IPlatform.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Log/ILogger.h>

namespace Accela::Platform
{
    /**
     * Abstract base class for desktop platform implementations.
     *
     * Uses SDL for files and text subsystems, relies on subclass to provide
     * events and windows subsystems.
     */
    class ACCELA_PUBLIC PlatformDesktop : public IPlatform
    {
        public:

            explicit PlatformDesktop(Common::ILogger::Ptr logger);

            [[nodiscard]] virtual bool Startup();
            virtual void Shutdown();

            [[nodiscard]] IFiles::Ptr GetFiles() const noexcept override;
            [[nodiscard]] IText::Ptr GetText() const noexcept override;

        protected:

            Common::ILogger::Ptr m_logger;

        private:

            IFiles::Ptr m_files;
            IText::Ptr m_text;
    };
}

#endif //LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PLATFORMDESKTOP_H
