/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_INFINITEPERLINNOISE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_INFINITEPERLINNOISE_H

#include "PerlinNoise.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <sstream>
#include <unordered_map>
#include <optional>
#include <utility>
#include <vector>

namespace Accela::Engine
{
    /**
     * Creates a source of infinite tiled perlin noise from which images
     * for specific sub-chunks can be queried for.
     */
    class InfinitePerlinNoise
    {
        public:

            struct PosKey
            {
                PosKey(int _x, int _y) : x(_x) , y(_y) { }
                int x; int y;

                bool operator==(const PosKey&) const = default;

                struct HashFunction {
                    std::size_t operator()(const PosKey& o) const {
                        std::stringstream ss;
                        ss << o.x << "-" << o.y;
                        return std::hash<std::string>{}(ss.str());
                    }
                };
            };

            struct ChunkKey : public PosKey { ChunkKey(int _x, int _y) : PosKey(_x, _y) {} };
            struct SubKey : public PosKey { SubKey(int _x, int _y) : PosKey(_x, _y) {} };

            using Keys = std::pair<ChunkKey, SubKey>;

            struct SubChunk
            {
                Keys keys;
                std::vector<float> chunkData;
            };

        public:

            /***
             * Creates a new source of infinite perlin noise. Perlin noise is generated
             * and tiled in perlinSize chunks. Each chunk is comprised of sub-chunks of
             * subSize size. perlinSize must be a clean multiple of subSize. The number of
             * sub-chunks within a chunk is (perlinSize/subSize)^2 . These sub-chunks can be
             * queried for via GetSubImage/GetSubImageIfNotExists, which return ImageDatas
             * of imageSize size representing the perlin noise in the sub chunk.
             *
             * All positions throughout the API are in X/Y space where X increases to the
             * right and Y increases downwards.
             *
             * @param perlinSize The size of perlin noise chunks to be generated
             * @param subSize The size of sub-chunks within chunks
             * @param imageSize The pixel size of images to generate from sub-chunks
             */
            InfinitePerlinNoise(unsigned int perlinSize,
                                unsigned int subSize,
                                unsigned int imageSize);

            /**
             * @return Whether or not a sub-chunk has previously been created for a specific position.
             */
            [[nodiscard]] bool SubExists(const glm::vec2& pos) const;

            /**
             * Returns the sub-chunk associated with a particular position. Will internally cache
             * the sub-chunk's data and will return from cache for subsequent calls.
             *
             * @param pos The point which identifies the sub-chunk which contains that point
             *
             * @return The sub-chunk containing the point, or std::nullopt on error.
             */
            [[nodiscard]] std::optional<SubChunk> GetSubChunk(const glm::vec2& pos);

            /**
             * Same as GetSubChunk except will also return std::nullopt if the sub-chunk data
             * is already cached internally.
             */
            [[nodiscard]] std::optional<SubChunk> GetSubChunkIfNotExists(const glm::vec2& pos);

            /**
             * Gets a list of all chunks (not sub-chunks) which are more than a specific distance from a
             * specific point.
             *
             * @param pos The position to query distance from
             * @param distance The query distance
             *
             * @return A list of all chunks with bounding points all at least distance from pos
             */
            [[nodiscard]] std::vector<ChunkKey> GetAllChunksOutsideDistance(const glm::vec2& pos, float distance) const;

            /**
             * Frees image data associated with a particular sub-chunk
             *
             * @param keys The keys that identify the sub-chunk
             */
            void FreeSubImage(const Keys& keys);

        private:

            struct Chunk
            {
                explicit Chunk(unsigned int perlinSize)
                    : perlinNoise(perlinSize)
                { }

                PerlinNoise perlinNoise;
                std::unordered_map<SubKey, SubChunk, SubKey::HashFunction> subs;
            };

        private:

            [[nodiscard]] Keys PosToKeys(const glm::vec2& pos) const;

            void EnsureChunk(const ChunkKey& chunkKey);

        private:

            unsigned int m_perlinSize;
            unsigned int m_subSize;
            unsigned int m_imageSize;

            unsigned int m_subsPerDimension;

            std::unordered_map<ChunkKey, Chunk, ChunkKey::HashFunction> m_chunks;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_INFINITEPERLINNOISE_H
