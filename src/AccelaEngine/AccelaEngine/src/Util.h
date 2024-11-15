/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_UTIL_H
#define LIBACCELAENGINE_SRC_UTIL_H

#include <Accela/Common/Build.h>

#include <string>
#include <optional>
#include <utility>

namespace Accela::Engine
{
    // Split a filename into name + extension
    [[nodiscard]] SUPPRESS_IS_NOT_USED static std::optional<std::pair<std::string, std::string>> SplitFileName(const std::string& fileName)
    {
        const auto periodPos = fileName.find_first_of('.');
        if (periodPos == std::string::npos || periodPos == fileName.length() - 1) { return std::nullopt; }

        return std::make_pair(
            fileName.substr(0, periodPos),
            fileName.substr(periodPos + 1, fileName.length() - periodPos - 1)
        );
    }
}

#endif //LIBACCELAENGINE_SRC_UTIL_H
