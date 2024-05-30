/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "QtFutureNotifier.h"

#include <QTimer>

namespace Accela
{

static constexpr unsigned int POLL_INTERVAL_MS = 10;

QtFutureNotifier::QtFutureNotifier(QObject* pParent)
    : QObject(pParent)
{
    OnTimer();
}

void QtFutureNotifier::OnTimer()
{
    if (!m_doRun) { return; }

    std::erase_if(m_entries, [](FutureEntry::UPtr& entry) {
        return entry->CheckAndEmit();
    });

    QTimer::singleShot(POLL_INTERVAL_MS, this, &QtFutureNotifier::OnTimer);
}

void QtFutureNotifier::Destroy()
{
    m_doRun = false;

    for (auto& entry : m_entries)
    {
        entry->BlockingWait();
    }
    m_entries.clear();
}

}
