#ifndef LIBACCELARENDERERVK_SRC_UTIL_FUTURES_H
#define LIBACCELARENDERERVK_SRC_UTIL_FUTURES_H

#include <future>

namespace Accela::Render
{
    template <typename T>
    [[nodiscard]] T PromiseResult(const T& result, std::promise<T>& promise)
    {
        promise.set_value(result);
        return result;
    }

    [[nodiscard]] inline bool SuccessResult(std::promise<bool>& promise)
    {
        return PromiseResult(true, promise);
    }

    [[nodiscard]] inline bool ErrorResult(std::promise<bool>& promise)
    {
        return PromiseResult(false,promise);
    }
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_FUTURES_H
