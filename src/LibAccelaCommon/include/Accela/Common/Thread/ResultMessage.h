#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_RESULTMESSAGE_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_RESULTMESSAGE_H

#include "Message.h"

#include <future>

namespace Accela::Common
{
    /**
     * Message which allows for a result to be asynchronously returned via a promise/future pair
     *
     * @tparam T The class type of the result to be returned
     */
    template <class T>
    class ResultMessage : public Message
    {
        public:

            using Ptr = std::shared_ptr<ResultMessage>;

        public:

            explicit ResultMessage(std::string typeIdentifier)
                : Message(std::move(typeIdentifier))
            { }

            /**
             * Call this on caller thread before sending the message to
             * get the future which holds the result of the message.
             *
             * Never call this more than once for a particular message.
             *
             * @return The future that will receive the result
             */
            std::future<T> CreateFuture()
            {
                return m_promise.get_future();
            }

            /**
             * Call to notify the caller thread of the result of the operation
             *
             * @param result The result to report
             */
            void SetResult(const T& result)
            {
                m_promise.set_value(result);
            }

            /**
             * Steals (moves out) the message's promise. If called, then other
             * methods in this class that deal with the promise (e.g. SetResult)
             * can never be called again.
             *
             * @return This message's promise
             */
            std::promise<T> StealPromise()
            {
                return std::move(m_promise);
            }

        private:

            std::promise<T> m_promise;
    };

    //
    // Specific Common ResultMessages
    //

    /**
     * A ResultMessage which returns a boolean result
     */
    struct BoolResultMessage : public Common::ResultMessage<bool>
    {
        BoolResultMessage()
            : Common::ResultMessage<bool>("BoolResultMessage")
        { }

        explicit BoolResultMessage(std::string typeIdentifier)
            : Common::ResultMessage<bool>(std::move(typeIdentifier))
        { }
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_RESULTMESSAGE_H
