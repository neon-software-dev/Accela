/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_UTIL_QTFUTURENOTIFIER_H
#define ACCELAEDITOR_UTIL_QTFUTURENOTIFIER_H

#include <Accela/Common/Thread/ResultMessage.h>

#include <QObject>

#include <future>

namespace Accela
{
    /**
     * Helper class which uses the Qt event loop to periodically poll futures
     * and emit specified signals when those futures have completed.
     */
    class QtFutureNotifier : public QObject
    {
        Q_OBJECT

        public:

            explicit QtFutureNotifier(QObject* pParent = nullptr);

        private:

            struct FutureEntry
            {
                using UPtr = std::unique_ptr<FutureEntry>;

                virtual ~FutureEntry() = default;
                virtual bool CheckAndEmit() = 0;
                virtual void BlockingWait() = 0;
            };

            template <typename T, typename C>
            struct FutureEntryT : FutureEntry
            {
                FutureEntryT(std::future<T> fut, C* caller, void(C::*fp)(const T&))
                    : m_fut(std::move(fut))
                    , m_caller(caller)
                    , m_sig(fp)
                {

                }

                bool CheckAndEmit() override
                {
                    if (m_fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                    {
                        (m_caller->*m_sig)(m_fut.get());
                        return true;
                    }

                    return false;
                }

                void BlockingWait() override
                {
                    (m_caller->*m_sig)(m_fut.get());
                }

                std::future<T> m_fut;
                C* m_caller;
                void(C::*m_sig)(const T&);
            };

        public:

            /**
             * Emits the specified signal when the provided future has finished completion
             *
             * @tparam T The result type of the future
             * @tparam C The class type to be signaled
             * @param fut The future to be monitored
             * @param caller The object instance to emit a signal to
             * @param fp The slot on caller to be signaled
             */
            template <typename T, typename C>
            void EmitWhenFinished(std::future<T> fut, C* caller, void(C::*fp)(const T&))
            {
                m_entries.emplace_back(std::make_unique<FutureEntryT<T, C>>(std::move(fut), caller, fp));
            }

            void Destroy();

        private slots:

            void OnTimer();

        private:

            bool m_doRun{true};

            std::vector<FutureEntry::UPtr> m_entries;
    };
}

#endif //ACCELAEDITOR_UTIL_QTFUTURENOTIFIER_H
