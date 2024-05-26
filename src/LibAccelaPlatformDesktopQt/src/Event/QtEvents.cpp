#include "QtEvents.h"

//#include <QCoreApplication>
#include <QAbstractEventDispatcher>

namespace Accela::Platform
{

QtEvents::QtEvents(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

std::queue<SystemEvent> QtEvents::PopSystemEvents()
{
    //QCoreApplication::processEvents();
    QAbstractEventDispatcher::instance()->processEvents(QEventLoop::ProcessEventsFlag::AllEvents);
    // TODO!
    return {};
}

}
