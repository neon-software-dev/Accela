/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_CONTAINER_CONCURRENTQUEUE_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_CONTAINER_CONCURRENTQUEUE_H

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <queue>
#include <string>
#include <condition_variable>
#include <set>
#include <optional>
#include <functional>
#include <algorithm>

namespace Accela::Common
{
    /**
     * A Queue which has full thread safety when accessed and manipulated by multiple threads.
     *
     * @tparam T The type of data stored in the queue. Must be copy constructable.
     */
    template <class T>
        requires std::is_copy_assignable_v<T>
    class ConcurrentQueue
    {
        public:

            /**
             * Push a new item into the queue.
             *
             * Will block while acquiring the queue mutex.
             */
            void Push(const T& item)
            {
                {
                    const std::lock_guard<std::mutex> dataLk(m_dataMutex);
                    m_data.push(item);
                }

                // Only notify after releasing the lock, so the other thread isn't
                // immediately blocked, waiting for us to let go.
                m_dataPushedCv.notify_one();
            }

            /**
             * Whether the queue is currently empty at the time of calling.
             *
             * Will block while acquiring the queue mutex.
             */
            [[nodiscard]] bool IsEmpty() const
            {
                const std::lock_guard<std::mutex> dataLk(m_dataMutex);

                return m_data.empty();
            }

            /**
             * Gets the size of the queue at the time of calling.
             *
             * @return The size of the queue
             */
            [[nodiscard]] std::size_t Size() const
            {
                const std::lock_guard<std::mutex> dataLk(m_dataMutex);

                return m_data.size();
            }

            /**
             * Sorts the queue by the given sort function
             *
             * @param sortFunc The comparison function to use for the sort
             */
            void Sort(const std::function<void(const T&, const T&)>& sortFunc)
            {
                const std::lock_guard<std::mutex> dataLk(m_dataMutex);

                std::ranges::sort(m_data, sortFunc);
            }

            /**
             * Returns a copy of the item at the top of the queue, if any.
             *
             * @return A copy of the item at the top of the queue, or std::nullopt
             * if the queue is empty at the time of calling.
             */
            [[nodiscard]] std::optional<T> TryPeek()
            {
                const std::lock_guard<std::mutex> dataLk(m_dataMutex);
                if (m_data.empty())
                {
                    return std::nullopt;
                }

                return m_data.front();
            }

            /**
             * Tries to pop an item off of the queue, if one exists.
             *
             * Will block while acquiring the queue mutex. Once the mutex is
             * acquired, will return immediately.
             *
             * @param item The popped item, if the method's output was True.
             * @return Whether an item was available to be popped.
             */
            [[nodiscard]] std::optional<T> TryPop()
            {
                const std::lock_guard<std::mutex> dataLk(m_dataMutex);
                if (m_data.empty())
                {
                    return std::nullopt;
                }

                auto item = m_data.front();
                m_data.pop();
                return item;
            }


            /**
             * Blocking call that blocks the calling thread until an item can be successfully popped from the
             * queue (or the optional timeout has expired).
             *
             * The blocked thread can be released from its waiting by a call to UnblockPopper() from a different
             * thread.
             *
             * Consumers which are waiting for a new item via BlockingPop are notified of new items in
             * round-robin fashion. Only one consumer is notified when the queue receives a new item.
             *
             * @param identifier A string that uniquely identifies the calling thread
             * @param timeout An optional maximum amount of time to wait for an item to be popped
             *
             * @return The popped item if an item could be popped, or std::nullopt if the timeout was hit
             * or if the wait was interrupted by a call to UnblockPopper.
             */
            [[nodiscard]] std::optional<T> BlockingPop(const std::string& identifier,
                                                       const std::optional<std::chrono::milliseconds>& timeout = std::nullopt)
            {
                // Predicate used to determine whether to stop waiting. We want to stop
                // waiting if we've been cancelled or if there's an item available to pop.
                const auto waitPredicate = [&] {
                    {
                        const std::lock_guard<std::mutex> setLk(m_unblockSetMutex);
                        if (m_unblockSet.find(identifier) != m_unblockSet.cend())
                        {
                            return true;
                        }
                    }

                    return !m_data.empty();
                };

                // Obtain a unique lock to access m_data
                std::unique_lock<std::mutex> dataLk(m_dataMutex);

                // If m_data has contents, pop an item off immediately and return it
                if (!m_data.empty())
                {
                    auto item = m_data.front();
                    m_data.pop();
                    return item;
                }

                // Otherwise, wait until there's an item available, the wait has been cancelled, or the
                // wait has timed out.
                bool timedOut = false;

                if (timeout.has_value())
                {
                    timedOut = !m_dataPushedCv.wait_for(dataLk, *timeout, waitPredicate);
                }
                else
                {
                    m_dataPushedCv.wait(dataLk, waitPredicate);
                }

                // Now that we're done waiting, check whether we're done because
                // we were cancelled
                {
                    const std::lock_guard<std::mutex> setLk(m_unblockSetMutex);
                    if (m_unblockSet.find(identifier) != m_unblockSet.cend())
                    {
                        // If we were cancelled, clear the cancel flag so that
                        // subsequent calls to BlockingPop work, and then bail out.
                        m_unblockSet.erase(identifier);
                        return std::nullopt;
                    }
                }

                // We waited, weren't cancelled, but the wait timed out, so return std::nullopt as no item was popped
                if (timedOut)
                {
                    return std::nullopt;
                }

                // We weren't cancelled, the wait didn't time out, so pop the available item
                auto item = m_data.front();
                m_data.pop();
                return item;
            }

            /**
             * Cancels/unblocks the blocking wait of a thread's previous call to BlockingPop()
             *
             * @param identifier The identifier that was registered in the BlockingPop() call
             */
            void UnblockPopper(const std::string& identifier)
            {
                {
                    const std::lock_guard<std::mutex> setLk(m_unblockSetMutex);
                    m_unblockSet.insert(identifier);
                }

                m_dataPushedCv.notify_all();
            }

        private:

            std::queue<T> m_data;                       // The queue of data being managed
            mutable std::mutex m_dataMutex;             // Used to synchronize access to m_data
            std::condition_variable m_dataPushedCv;     // Used to notify threads of newly pushed data

            std::set<std::string> m_unblockSet;     // Entries represent cancelled BlockingPop calls
            mutable std::mutex m_unblockSetMutex;   // Used to synchronize access to m_unblockSet
    };
}


#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_CONTAINER_CONCURRENTQUEUE_H
