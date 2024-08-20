/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Extra/InfinitePerlinNoise.h>

#include <cassert>
#include <algorithm>

namespace Accela::Engine
{

InfinitePerlinNoise::InfinitePerlinNoise(unsigned int perlinSize,
                                         unsigned int subSize,
                                         unsigned int imageSize)
    : m_perlinSize(perlinSize)
    , m_subSize(subSize)
    , m_imageSize(imageSize)
{
    assert(m_perlinSize % m_subSize == 0);

    m_subsPerDimension = m_perlinSize / m_subSize;
}

std::pair<InfinitePerlinNoise::ChunkKey, InfinitePerlinNoise::SubKey> InfinitePerlinNoise::PosToKeys(const glm::vec2& pos) const
{
    //
    // Chunk
    //
    const int chunkX = std::floor(pos.x / (float)m_perlinSize);
    const int chunkY = std::floor(pos.y / (float)m_perlinSize);

    const auto chunkKey = ChunkKey(chunkX, chunkY);

    //
    // Sub
    //
    const glm::vec2 chunkPos = {(float)chunkX * (float)m_perlinSize, (float)chunkY * (float)m_perlinSize};
    const glm::vec2 chunkPosToPos = pos - chunkPos;

    const int subX = std::min((int)m_subsPerDimension - 1, (int)std::floor(chunkPosToPos.x / (float)m_subSize));
    const int subY = std::min((int)m_subsPerDimension - 1, (int)std::floor(chunkPosToPos.y / (float)m_subSize));

    const auto subKey = SubKey(subX, subY);

    return {chunkKey, subKey};
}

void InfinitePerlinNoise::EnsureChunk(const ChunkKey& chunkKey)
{
    // Nothing to do if the chunk already exists
    if (m_chunks.contains(chunkKey))
    {
        return;
    }

    // Keys of the chunks that surround the chunk
    const auto chunkLeft = ChunkKey(chunkKey.x - 1, chunkKey.y);
    const auto chunkRight = ChunkKey(chunkKey.x + 1, chunkKey.y);
    const auto chunkTop = ChunkKey(chunkKey.x, chunkKey.y - 1);
    const auto chunkBottom = ChunkKey(chunkKey.x, chunkKey.y + 1);

    // Create the chunk
    Chunk chunk(m_perlinSize);

    // Overwrite the chunk's edges to the edge values of any surrounding chunks
    // that may exist
    if (m_chunks.contains(chunkLeft))
    {
        (void)chunk.perlinNoise.SetSideGradients(
            PerlinNoise::Side::Left,
            m_chunks.at(chunkLeft).perlinNoise,
            PerlinNoise::Side::Right
        );
    }

    if (m_chunks.contains(chunkRight))
    {
        (void)chunk.perlinNoise.SetSideGradients(
            PerlinNoise::Side::Right,
            m_chunks.at(chunkRight).perlinNoise,
            PerlinNoise::Side::Left
        );
    }

    if (m_chunks.contains(chunkTop))
    {
        (void)chunk.perlinNoise.SetSideGradients(
            PerlinNoise::Side::Top,
            m_chunks.at(chunkTop).perlinNoise,
            PerlinNoise::Side::Bottom
        );
    }

    if (m_chunks.contains(chunkBottom))
    {
        (void)chunk.perlinNoise.SetSideGradients(
            PerlinNoise::Side::Bottom,
            m_chunks.at(chunkBottom).perlinNoise,
            PerlinNoise::Side::Top
        );
    }

    // Record the new chunk
    m_chunks.insert({chunkKey, chunk});
}

bool InfinitePerlinNoise::SubExists(const glm::vec2& pos) const
{
    const auto keys = PosToKeys(pos);

    const auto it = m_chunks.find(keys.first);
    if (it == m_chunks.cend())
    {
        return false;
    }

    const auto it2 = it->second.subs.find(keys.second);
    if (it2 == it->second.subs.cend())
    {
        return false;
    }

    return true;
}

std::optional<InfinitePerlinNoise::SubChunk> InfinitePerlinNoise::GetSubChunk(const glm::vec2& pos)
{
    const auto keys = PosToKeys(pos);
    const auto& chunkKey = keys.first;
    const auto& subKey = keys.second;

    // Ensure that the query chunk exists
    EnsureChunk(chunkKey);
    auto& chunk = m_chunks.at(chunkKey);

    // Check if the chunk already contains that sub, and return it if so
    const auto it = chunk.subs.find(subKey);
    if (it != chunk.subs.cend())
    {
        return it->second;
    }

    // Otherwise, generate the sub image
    const float subXOffset = (float)subKey.x * (float)m_subSize;
    const float subYOffset = (float)subKey.y * (float)m_subSize;

    auto subData = chunk.perlinNoise.Get(
        {(unsigned int)subXOffset, (unsigned int)subYOffset},
        m_subSize,
        m_imageSize
    );
    if (subData == std::nullopt)
    {
        return std::nullopt;
    }

    // Record the sub and return
    auto subChunk = SubChunk(keys, *subData);

    chunk.subs.insert({subKey, subChunk});

    return subChunk;
}

std::optional<InfinitePerlinNoise::SubChunk> InfinitePerlinNoise::GetSubChunkIfNotExists(const glm::vec2& pos)
{
    if (SubExists(pos)) { return std::nullopt; }

    return GetSubChunk(pos);
}

std::vector<InfinitePerlinNoise::ChunkKey> InfinitePerlinNoise::GetAllChunksOutsideDistance(const glm::vec2& pos, float distance) const
{
    std::vector<ChunkKey> chunks;

    for (const auto& chunk : m_chunks)
    {
        const std::vector<glm::vec2> chunkPoints = {
            {chunk.first.x, chunk.first.y},
            {chunk.first.x + m_perlinSize, chunk.first.y},
            {chunk.first.x + m_perlinSize, chunk.first.y + m_perlinSize},
            {chunk.first.x, chunk.first.y + m_perlinSize}
        };

        if (std::ranges::all_of(chunkPoints, [&](const auto& chunkPoint){
            return glm::distance(chunkPoint, pos) > distance;
        }))
        {
            chunks.push_back(chunk.first);
        }
    }

    return chunks;
}

void InfinitePerlinNoise::FreeSubImage(const Keys& keys)
{
    const auto it = m_chunks.find(keys.first);
    if (it == m_chunks.cend())
    {
        return;
    }

    // Erase the sub-chunk
    it->second.subs.erase(keys.second);

    // If the chunk itself is now empty, erase it too
    if (it->second.subs.empty())
    {
        m_chunks.erase(keys.first);
    }
}

}
