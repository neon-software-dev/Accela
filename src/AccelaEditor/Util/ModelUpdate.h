/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_UTIL_MODELUPDATE_H
#define ACCELAEDITOR_UTIL_MODELUPDATE_H

#include <QMetaObject>

namespace Accela
{
    /**
     * Helper function used to both update a field in a model object and emit a signal
     * about the changed field (if the field's value has changed)
     *
     * @tparam T The type of variable in the model to be updated
     * @tparam C The class type of the caller
     *
     * @param dest The model variable to be updated
     * @param value The new value for the model variable
     * @param caller The caller of this function
     * @param fp The function to be signalled
     * @param force If true, will emit even if the value being updated hasn't changed
     *
     * @return Whether the update signal was emitted
     */
    template <typename T, typename C>
    bool UpdateAndEmit(T& dest, const T& value, C* caller, void(C::*fp)(const T&), bool force = false)
    {
        if (dest == value && !force) { return false; }

        dest = value;
        (caller->*fp)(value);

        return true;
    }
}

#endif //ACCELAEDITOR_UTIL_MODELUPDATE_H
