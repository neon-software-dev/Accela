/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDSOURCE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDSOURCE_H

#include "Id.h"

#include <mutex>
#include <unordered_set>

namespace Accela::Render
{
    /**
     * Provides integral ids which can be returned to the source and reused later. Thread-safe.
     *
     * @tparam T The data type to use for ids
     */
    template <class T>
    class IdSource
    {
        public:

            T GetId()
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                // Pull from the set of returned ids first
                if (!m_freeIds.empty())
                {
                    T result = *m_freeIds.begin();
                    m_freeIds.erase(m_freeIds.begin());
                    return result;
                }

                // If no returned ids exist, create and return a new ID
                return T(++m_id);
            }

            void ReturnId(const T& id)
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                m_freeIds.insert(id);
            }

            void Reset()
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                m_id = INVALID_ID;
                m_freeIds = {};
            }

        private:

            std::mutex m_mutex;
            IdType m_id{INVALID_ID}; // Current max id that's been created
            std::unordered_set<T> m_freeIds; // Ids that have been returned to the source
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDSOURCE_H
