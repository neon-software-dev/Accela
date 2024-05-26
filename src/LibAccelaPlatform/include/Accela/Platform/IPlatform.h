#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_IPLATFORM_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_IPLATFORM_H

#include "Event/IEvents.h"
#include "File/IFiles.h"
#include "Text/IText.h"
#include "Window/IWindow.h"
#include "VR/IVR.h"

#include <memory>

namespace Accela::Platform
{
    /**
     * Main interface to the platform system. Provides an OS-agnostic interface to
     * OS functionality such as loading files, accessing window details, accessing
     * system events, etc.
     */
    class IPlatform
    {
        public:

            using Ptr = std::shared_ptr<IPlatform>;

        public:

            virtual ~IPlatform() = default;

            [[nodiscard]] virtual IEvents::Ptr GetEvents() const noexcept = 0;
            [[nodiscard]] virtual IFiles::Ptr GetFiles() const noexcept = 0;
            [[nodiscard]] virtual IText::Ptr GetText() const noexcept = 0;
            [[nodiscard]] virtual IWindow::Ptr GetWindow() const noexcept = 0;
            [[nodiscard]] virtual IVR::Ptr GetVR() const noexcept = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_IPLATFORM_H
