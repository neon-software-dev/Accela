/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_UTIL_POLLINGMESSAGEFULFILLER_H
#define ACCELAEDITOR_UTIL_POLLINGMESSAGEFULFILLER_H

#include <Accela/Common/Thread/ResultMessage.h>

#include <vector>
#include <future>
#include <memory>

namespace Accela
{
    /**
     * Helper class which uses a polling mechanism to fulfill ResultMessages when corresponding
     * futures have completed. Relies on external logic to call FulfillFinished on a regular interval
     * to check for finished message futures.
     */
    class PollingMessageFulfiller
    {
        private:

            struct FutureEntry
            {
                using UPtr = std::unique_ptr<FutureEntry>;

                virtual ~FutureEntry() = default;
                virtual bool CheckAndFulfill() = 0;
                virtual void BlockingWait() = 0;
            };

            template <typename T>
            struct FutureEntryT : FutureEntry
            {
                FutureEntryT(std::future<T> fut, Common::ResultMessage<T>::Ptr msg)
                    : m_fut(std::move(fut))
                    , m_msg(std::move(msg))
                { }

                bool CheckAndFulfill() override
                {
                    if (m_fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                    {
                        m_msg->SetResult(m_fut.get());
                        return true;
                    }

                    return false;
                }

                void BlockingWait() override
                {
                    m_msg->SetResult(m_fut.get());
                }

                std::future<bool> m_fut;
                Common::ResultMessage<T>::Ptr m_msg;
            };

        public:

            /**
             * Sets msg's result with the result of the provided future, when the future has finished.
             */
            template <typename T>
            void FulfillWhenFinished(Common::ResultMessage<T>::Ptr msg, std::future<T> fut)
            {
                m_entries.emplace_back(std::make_unique<FutureEntryT<T>>(std::move(fut), msg));
            }

            /**
             * Polling command which checks for and processes finished futures.
             */
            void FulfillFinished();

            /**
             * Blocking wait for all futures to finish
             */
            void BlockingWaitForAll();

        private:

            std::vector<FutureEntry::UPtr> m_entries;
    };
}

#endif //ACCELAEDITOR_UTIL_POLLINGMESSAGEFULFILLER_H
