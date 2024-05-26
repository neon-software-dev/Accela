#include <Accela/Common/Thread/MessageDrivenThreadPool.h>
#include <Accela/Common/Thread/ThreadUtil.h>

#include <sstream>

namespace Accela::Common
{

MessageDrivenThreadPool::MessageDrivenThreadPool(std::string tag,
                                                 unsigned int poolSize,
                                                 std::optional<MessageHandler> msgHandler,
                                                 std::optional<IdleHandler> idleHandler)
    : m_tag(std::move(tag))
    , m_poolSize(poolSize)
    , m_msgHandler(std::move(msgHandler))
    , m_idleHandler(std::move(idleHandler))
    , m_msgQueue(std::make_unique<ConcurrentQueue<EnqueuedMessage>>())
{
    for (unsigned int x = 0; x < m_poolSize; ++x)
    {
        const auto threadIdentifier = GetThreadIdentifier(x);

        auto thread = std::thread(&MessageDrivenThreadPool::MessageReceiverThreadFunc, this, threadIdentifier);

        Common::SetThreadName(thread.native_handle(), threadIdentifier);

        m_threads.emplace_back(std::move(thread));
    }
}

MessageDrivenThreadPool::~MessageDrivenThreadPool()
{
    // Negate the run flag, prompting all threads to finish
    m_run = false;

    // Unblock all threads from waiting for new messages, allowing them to see
    // the new run state
    for (unsigned int x = 0; x < m_poolSize; ++x)
    {
        m_msgQueue->UnblockPopper(GetThreadIdentifier(x));
    }

    // Now wait for all threads to finish
    for (auto& thread : m_threads)
    {
        thread.join();
    }
}

void MessageDrivenThreadPool::PostMessage(const Message::Ptr& message, const std::optional<MessageHandler>& messageHandler)
{
    m_msgQueue->Push(EnqueuedMessage(message, messageHandler));
}

void MessageDrivenThreadPool::MessageReceiverThreadFunc(const std::string& threadIdentifier)
{
    std::optional<std::chrono::milliseconds> maxWait;

    if (m_idleHandler)
    {
        maxWait = std::chrono::milliseconds(50);
    }

    while (m_run)
    {
        EnqueuedMessage msg{};

        // Wait until either we receive a message to be handled, or the queue has stopped us
        // from listening due to a UnblockPopper call when this class is destructed, or
        // max wait time has expired
        const bool popResult = m_msgQueue->BlockingPop(msg, threadIdentifier, maxWait);

        // Check whether we were told to stop, and stop the thread if so.
        if (!m_run) { return; }

        // Otherwise, if there was a message popped, process it
        if (popResult)
        {
            // If the message has a handler set, invoke it
            if (msg.handler)
            {
                (*msg.handler)(msg.message);
            }
            // Otherwise, if a global message handler was provided, invoke it
            else if (m_msgHandler)
            {
                (*m_msgHandler)(msg.message);
            }
        }
        // Otherwise, if the pop timed out, give the handler an idle callback
        else if (m_idleHandler)
        {
            (*m_idleHandler)();
        }
    }
}

std::string MessageDrivenThreadPool::GetThreadIdentifier(unsigned int threadIndex)
{
    std::stringstream ss;
    ss << "MTP" << threadIndex << "-" << m_tag;
    return ss.str();
}

}
