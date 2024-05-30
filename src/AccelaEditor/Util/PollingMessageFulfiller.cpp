/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PollingMessageFulfiller.h"

namespace Accela
{

void PollingMessageFulfiller::FulfillFinished()
{
    std::erase_if(m_entries, [](FutureEntry::UPtr& entry) {
        return entry->CheckAndFulfill();
    });
}

void PollingMessageFulfiller::BlockingWaitForAll()
{
    for (auto& entry : m_entries)
    {
        entry->BlockingWait();
    }
    m_entries.clear();
}

}
