/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Util/PerlinNoise.h>

#include <random>

namespace Accela::Engine
{

static inline float Lerp(float a, float b, float w) {
    return (w * (b - a)) + a;
}

static inline float SCurve(float t)
{
    return t*t*(3-2*t);
}

PerlinNoise::PerlinNoise(unsigned int size)
    : m_size(size)
    , m_gridSize(m_size + 1)
{
    GenerateGridVectors();
}

float PerlinNoise::operator()(const glm::vec2& p) const
{
    return Get(p);
}

void PerlinNoise::GenerateGridVectors()
{
    std::random_device m_rd;
    std::mt19937 m_mt{m_rd()};

    //
    // Generate an m_gridSize x m_gridSize grid of random 2D unit vectors
    //
    m_grid = std::vector<glm::vec2>(m_gridSize * m_gridSize, {0,0});

    for (unsigned int y = 0; y < m_gridSize; ++y)
    {
        for (unsigned int x = 0; x < m_gridSize; ++x)
        {
            m_grid[x + (y * m_gridSize)] = glm::normalize(glm::vec2{
                std::uniform_real_distribution<float>(-1.0f, 1.0f)(m_mt),
                std::uniform_real_distribution<float>(-1.0f, 1.0f)(m_mt)
            });
        }
    }
}

float PerlinNoise::Get(const glm::vec2& p) const
{
    if (p.x < 0.0f) { return 0.0f; }
    if (p.x > (float)m_size) { return 0.0f; }
    if (p.y < 0.0f) { return 0.0f; }
    if (p.y > (float)m_size) { return 0.0f; }

    // Top-Left X/Y coordinates of the cell that p is contained within
    unsigned int pCellX = std::floor(p.x);
    unsigned int pCellY = std::floor(p.y);

    // When querying for exactly the right and bottom edges, drop the cell index up/left by
    // one to consider it in that cell, otherwise we'd need to look into grid cells to the
    // right and below of the grid we've created
    if (pCellX == m_size) { pCellX--; }
    if (pCellY == m_size) { pCellY--; }

    // Index of the pCell in the grid vector
    const unsigned int pCellIndex =  pCellX + (pCellY * m_gridSize);

    // Fetch the grid gradients of the cell's four bounding points.
    // Note the clockwise ordering.
    const auto& gv1 = m_grid[pCellIndex];
    const auto& gv2 = m_grid[pCellIndex + 1];
    const auto& gv3 = m_grid[pCellIndex + 1 + m_gridSize];
    const auto& gv4 = m_grid[pCellIndex + m_gridSize];

    // Calculate the offset vectors pointing from each bounding point to the query point
    const auto ov1 = p - glm::vec2(pCellX, pCellY);
    const auto ov2 = p - glm::vec2(pCellX + 1, pCellY);
    const auto ov3 = p - glm::vec2(pCellX + 1, pCellY + 1);
    const auto ov4 = p - glm::vec2(pCellX, pCellY + 1);

    // Calculate the dot products of each bounding point's random gradient with the offset
    // vector from that bounding point to the query point
    const auto d1 = glm::dot(gv1, ov1);
    const auto d2 = glm::dot(gv2, ov2);
    const auto d3 = glm::dot(gv3, ov3);
    const auto d4 = glm::dot(gv4, ov4);

    // X/Y percentages (0.0..1.0) of the query point's position within its cell
    const float xPercent = p.x - (float)pCellX;
    const float yPercent = p.y - (float)pCellY;

    // S-curved X/Y percentages
    const float xS = SCurve(xPercent);
    const float yS = SCurve(yPercent);

    // Lerp the calculated dot products in X directions then in Y direction
    const auto topXLerp = Lerp(d1, d2, xS);
    const auto bottomXLerp = Lerp(d4, d3, xS); // Note the correction for clockwise ordering
    const auto yLerp = Lerp(topXLerp, bottomXLerp, yS);

    return yLerp;
}

std::vector<glm::vec2> PerlinNoise::GetSideGradients(PerlinNoise::Side side) const
{
    std::vector<glm::vec2> results;
    results.reserve(m_gridSize);

    switch (side)
    {
        case Side::Left:
        {
            for (unsigned int y = 0; y < m_gridSize; ++y)
            {
                results.push_back(m_grid[y * m_gridSize]);
            }
        }
        break;
        case Side::Right:
        {
            for (unsigned int y = 0; y < m_gridSize; ++y)
            {
                results.push_back(m_grid[(y * m_gridSize) + (m_gridSize - 1)]);
            }
        }
        break;
        case Side::Top:
        {
            for (unsigned int x = 0; x < m_gridSize; ++x)
            {
                results.push_back(m_grid[x]);
            }
        }
        break;
        case Side::Bottom:
        {
            for (unsigned int x = 0; x < m_gridSize; ++x)
            {
                results.push_back(m_grid[x + ((m_gridSize - 1) * m_gridSize)]);
            }
        }
        break;
    }

    return results;
}

bool PerlinNoise::SetSideGradients(Side destSide, const PerlinNoise& sourceNoise, Side sourceSide)
{
    if (m_gridSize != sourceNoise.m_gridSize)
    {
        return false;
    }

    const auto gradients = sourceNoise.GetSideGradients(sourceSide);

    switch (destSide)
    {
        case Side::Left:
        {
            for (unsigned int y = 0; y < m_gridSize; ++y)
            {
                m_grid[y * m_gridSize] = gradients[y];
            }
        }
        break;
        case Side::Right:
        {
            for (unsigned int y = 0; y < m_gridSize; ++y)
            {
                m_grid[(y * m_gridSize) + (m_gridSize - 1)] = gradients[y];
            }
        }
        break;
        case Side::Top:
        {
            for (unsigned int x = 0; x < m_gridSize; ++x)
            {
                m_grid[x] = gradients[x];
            }
        }
        break;
        case Side::Bottom:
        {
            for (unsigned int x = 0; x < m_gridSize; ++x)
            {
                m_grid[x + ((m_gridSize - 1) * m_gridSize)] = gradients[x];
            }
        }
        break;
    }

    return true;
}

std::optional<std::vector<float>> PerlinNoise::Get(const std::pair<unsigned int, unsigned int>& queryOffset,
                                                   const unsigned int& querySize,
                                                   const unsigned int& dataSize) const
{
    // Check for querying outside the bounds of the perlin grid
    if (queryOffset.first + querySize > m_size) { return std::nullopt; }
    if (queryOffset.second + querySize > m_size) { return std::nullopt; }

    // The interval of query points within the query section needed to match the data size
    const float interval = (float)querySize / (float)(dataSize - 1);

    std::vector<float> data;
    data.reserve(dataSize * dataSize);

    for (unsigned int y = 0; y < dataSize; ++y)
    {
        for (unsigned int x = 0; x < dataSize; ++x)
        {
            // Query for the perlin value
            data.push_back(Get({
                (float)queryOffset.first + (float)x * interval,
                (float)queryOffset.second + (float)y * interval}
            ));
        }
    }

    return data;
}

std::optional<std::vector<float>> PerlinNoise::Get(const std::pair<unsigned int, unsigned int>& queryOffset,
                                                   const std::vector<std::pair<unsigned int, float>>& octaves,
                                                   const unsigned int& dataSize) const
{
    //
    // Query for the perlin data for each octave
    //
    std::vector<std::vector<float>> octaveData;

    for (const auto& octave: octaves)
    {
        const auto dataOpt = Get(queryOffset, octave.first, dataSize);
        if (!dataOpt)
        {
            return std::nullopt;
        }

        octaveData.push_back(*dataOpt);
    }

    //
    // Combine the octave data into a result data set
    //
    std::vector<float> result(dataSize * dataSize, 0.0f);

    for (unsigned int o = 0; o < octaves.size(); ++o)
    {
        const float octaveAmplitude = octaves[o].second;

        for (std::size_t x = 0; x < dataSize * dataSize; ++x)
        {
            result[x] += octaveAmplitude * octaveData[o][x];
        }
    }

    //
    // Normalize the output back to [-1,1] range
    //
    float amplitudeTotal = 0.0f;

    for (const auto& octave : octaves)
    {
        amplitudeTotal += octave.second;
    }

    for (unsigned int x = 0; x < dataSize; ++x)
    {
        result[x] = result[x] / amplitudeTotal;
    }

    return result;
}

Common::ImageData::Ptr PerlinNoise::ToRGBA32(const std::vector<float>& data)
{
    const auto dataSize = (unsigned int)std::sqrt(data.size());

    std::vector<std::byte> dataBytes;
    dataBytes.reserve(data.size());

    for (const auto& val : data)
    {
        // Convert from [-1,1] -> [0,1]
        const float rangedVal = (val + 1.0f) / 2.0f;

        // Convert from [0,1] -> [0,255]
        const auto imageByte = std::byte(rangedVal * 255.0f);

        dataBytes.push_back(imageByte);        // R
        dataBytes.push_back(imageByte);        // G
        dataBytes.push_back(imageByte);        // B
        dataBytes.push_back(std::byte(255));   // A
    }

    return std::make_shared<Common::ImageData>(
        dataBytes,
        1,
        dataSize,
        dataSize,
        Common::ImageData::PixelFormat::RGBA32
    );
}

}
