/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_PERLINNOISE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_PERLINNOISE_H

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/ImageData.h>

#include <glm/glm.hpp>

#include <vector>
#include <utility>
#include <optional>

namespace Accela::Engine
{
    /**
     * Simple implementation of perlin noise.
     *
     * All coordinates assume a 2D grid/image aligned in the X/Y plane, with
     * 0,0 being the top left point of chunk 0, with x increasing to right, and
     * y increasing downwards.
     */
    class ACCELA_PUBLIC PerlinNoise
    {
        public:

            /** Identifies a specific side of the perlin noise data */
            enum class Side { Left, Right, Top, Bottom };

        public:

            /**
             * Create a new source of random perlin noise, that's size units
             * large in width and height.
             *
             * @param size The side length / size of the perlin noise.
             */
            explicit PerlinNoise(unsigned int size);

            /**
            * @return The cell size of the perlin noise, as supplied to the constructor.
            */
            [[nodiscard]] unsigned int GetSize() const { return m_size; }

            /** See: Get(..) */
            [[nodiscard]] float operator()(const glm::vec2& p) const;

            /**
             * Query for the noise value at a specific query coordinate.
             *
             * @param p The coordinate to query. Must be in the range [0,m_size]
             *
             * @return The perlin noise value, in the range of [-1,1], or zero if
             * p was out of bounds
             */
            [[nodiscard]] float Get(const glm::vec2& p) const;

            /**
             * Query for noise values within a particular subsection. Will look at the noise
             * that's querySize x querySize large at offset queryOffset, and will fetch
             * getSize x getSize values from that query subset.
             *
             * @param queryOffset The offset into the noise to query. Must be in the range [[0,0], [m_size,m_size])
             * @param querySize The size to query for data from. . Must be in the range ([0,0], [m_size,m_size])
             * @param dataSize The size of data points to create from the query subset
             *
             * @return The data points, or std::nullopt if out of range input parameters
             */
            [[nodiscard]] std::optional<std::vector<float>> Get(
                const std::pair<unsigned int, unsigned int>& queryOffset,
                const unsigned int& querySize,
                const unsigned int& dataSize
            ) const;

            /**
             * Same as the above Get(..) method except instead of one query size, takes in a number
             * of query octaves, where each octave has its own query size and amplitude.
             *
             * @param queryOffset The offset into the noise to query. Must be in the range [[0,0], [m_size,m_size])
             * @param octaves Octave data. Each entry is a (querySize, amplitude) pair.
             * @param dataSize The size of data points to create from the query subset
             *
             * @return The data points, or std::nullopt if out of range input parameters
             */
            [[nodiscard]] std::optional<std::vector<float>> Get(
                const std::pair<unsigned int, unsigned int>& queryOffset,
                const std::vector<std::pair<unsigned int, float>>& octaves,
                const unsigned int& dataSize
            ) const;

            /**
             * Set the gradient values along one side equal to the gradient values from
             * a source perlin noise's side. Used to connect two PerlinNoises together
             * along their shared side. The sizes of both PerlinNoises, this and the source,
             * must match.
             *
             * @param destSide The side of this PerlinNoise to update
             * @param sourceNoise The PerlinNoise source for new gradients
             * @param sourceSide The side of sourceNoise from which to copy gradients
             *
             * @return True on success, false if gradients.size() is not equal to GetGridSize()
             */
            [[nodiscard]] bool SetSideGradients(Side destSide, const PerlinNoise& sourceNoise, Side sourceSide);

            /**
             * Simple helper function that converts perlin noise data to an RGBA32 format ImageData. Sets
             * the R,G, and B values of each pixel to the [0..255] mapped data value, and the A value to 255.
             *
             * @param data Perlin noise data fetched via a call to Get(..)
             *
             * @return an RGBA32 format ImageData representing the data
             */
            [[nodiscard]] static Common::ImageData::Ptr ToRGBA32(const std::vector<float>& data);

        private:

            [[nodiscard]] unsigned int GetGridSize() const { return m_gridSize; }

            [[nodiscard]] std::vector<glm::vec2> GetSideGradients(Side side) const;

            void GenerateGridVectors();

        private:

            // User supplied, e.g. 2x2 means they can query between [0..2] for x and y
            unsigned int m_size;

            // Grid size: a 2x2 m_size grid is made up of 3x3 points
            unsigned int m_gridSize;

            std::vector<glm::vec2> m_grid;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_PERLINNOISE_H
