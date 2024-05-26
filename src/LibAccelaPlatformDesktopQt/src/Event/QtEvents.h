#ifndef LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTEVENTS_H
#define LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTEVENTS_H

#include <Accela/Platform/Event/IEvents.h>

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Platform
{
    class QtEvents : public IEvents
    {
        public:

            explicit QtEvents(Common::ILogger::Ptr logger);

            [[nodiscard]] std::queue<SystemEvent> PopSystemEvents() override;

        private:

            Common::ILogger::Ptr m_logger;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTEVENTS_H
