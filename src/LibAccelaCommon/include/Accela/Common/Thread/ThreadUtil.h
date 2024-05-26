/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_THREADUTIL_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_THREADUTIL_H

#include "../BuildInfo.h"

#include <future>
#include <thread>
#include <string>

namespace Accela::Common
{
    /**
     * Creates a future which already has a value immediately available
     */
    template <typename T>
    std::future<T> ImmediateFuture(const T& value)
    {
        std::promise<T> promise;
        std::future<T> future = promise.get_future();
        promise.set_value(value);
        return future;
    }

    /**
     * Sets the name of the provided native thread handle. OS dependent. Currently only works
     * on Linux.
     */
    void SetThreadName(std::thread::native_handle_type threadHandle, const std::string& name);
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_THREADUTIL_H
