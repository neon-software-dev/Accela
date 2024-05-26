#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_CONTAINER_CONCURRENTQUEUE_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_CONTAINER_CONCURRENTQUEUE_H

#include <memory>
#include <queue>
#include <string>
#include <condition_variable>
#include <set>
#include <optional>

namespace Accela::Common
{
    /**
     * A Queue which has full thread safety when accessed and manipulated by multiple threads.
     *
     * @tparam T The type of data stored in the queue. Must be copy constructable.
     */
    template <class T>
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
                    const std::lock_guard<std::mutex> dataLk(m_dataCvMutex);
                    m_data.push(item);
                }

                // Only notify after releasing the lock, so the other thread isn't
                // immediately blocked, waiting for us to let go.
                m_dataCv.notify_one();
            }

            /**
             * Whether the queue is currently empty.
             *
             * Will block while acquiring the queue mutex.
             */
            bool IsEmpty() const
            {
                const std::lock_guard<std::mutex> dataLk(m_dataCvMutex);

                return m_data.empty();
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
            bool TryPop(T& item)
            {
                const std::lock_guard<std::mutex> dataLk(m_dataCvMutex);
                if (m_data.empty())
                {
                    return false;
                }

                item = m_data.front();
                m_data.pop();
                return true;
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
             * @param item The popped item.
             * @param identifier A string that uniquely identifies the calling thread
             * @param timeout An optional maximum amount of time to wait for an item to be popped
             *
             * @return True if an item was popped and returned. If false, then the Pop was
             * either canceled via an UnblockPopper() call or the block timed out; in both cases the
             * item param is left unmodified.
             */
            bool BlockingPop(T& item,
                             const std::string& identifier,
                             const std::optional<std::chrono::milliseconds>& timeout = std::nullopt)
            {
                // Predicate used to determine whether to stop waiting. We want to stop
                // waiting if we've been cancelled or if there's an item available to pop.
                const auto waitPredicate =  [&] {
                    {
                        const std::lock_guard<std::mutex> setLk(m_unblockSetMutex);
                        if (m_unblockSet.find(identifier) != m_unblockSet.cend())
                        {
                            return true;
                        }
                    }

                    return !m_data.empty();
                };

                //
                // Wait until there's an item available, the wait has been cancelled, or the
                // wait has timed out.
                //
                bool timedOut = false;

                std::unique_lock<std::mutex> dataLk(m_dataCvMutex);

                if (timeout.has_value())
                {
                    timedOut = !m_dataCv.wait_for(dataLk, *timeout, waitPredicate);
                }
                else
                {
                    m_dataCv.wait(dataLk, waitPredicate);
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
                        return false;
                    }
                }

                // We waited, weren't cancelled, but the wait timed out, so return false as no item was popped
                if (timedOut)
                {
                    return false;
                }

                // We weren't cancelled, the wait didn't time out, so pop the available item
                item = m_data.front();
                m_data.pop();
                return true;
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

                m_dataCv.notify_all();
            }

        private:

            std::queue<T> m_data;                   // The queue of data being managed
            std::condition_variable m_dataCv;       // Used to notify threads of new m_data events
            mutable std::mutex m_dataCvMutex;       // Used to synchronize access to m_data

            std::set<std::string> m_unblockSet;     // Entries represent cancelled BlockingPop calls
            mutable std::mutex m_unblockSetMutex;   // Used to synchronize access to m_unblockSet
    };
}


#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_CONTAINER_CONCURRENTQUEUE_H
