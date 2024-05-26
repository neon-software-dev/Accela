#include <Accela/Common/Thread/ThreadUtil.h>

namespace Accela::Common
{

// TODO: Windows support
void SetThreadName(std::thread::native_handle_type threadHandle, const std::string& name)
{
    std::string threadName = name;

    if (threadName.length() > 15)
    {
        threadName = threadName.substr(0, 15);
    }

    #if __linux__ || __unix__
        pthread_setname_np(threadHandle, threadName.c_str());
    #else
        (void)threadHandle;
        (void)threadName;
    #endif
}

}
