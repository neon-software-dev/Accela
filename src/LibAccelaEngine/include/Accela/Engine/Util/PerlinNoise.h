/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_PERLINNOISE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_PERLINNOISE_H

#include <Accela/Common/ImageData.h>

#include <glm/glm.hpp>

#include <vector>
#include <utility>

namespace Accela::Engine
{
    /**
     * Simple implementation of perlin noise.
     *
     * All coordinates assume a 2D grid/image aligned in the X/Y plane, with
     * 0,0 being the top left point, x increasing to right, and y increasing
     * downwards.
     */
    class PerlinNoise
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

            /** See: Get(..) */
            [[nodiscard]] float operator()(const glm::vec2& p) const;

            /**
             * Query for the noise value at a specific query coordinate.
             *
             * @param p The coordinate to query. Must be in the range [0,m_size]
             *
             * @return The perlin noise value, or zero if p was out of bounds
             */
            [[nodiscard]] float Get(const glm::vec2& p) const;

            /**
             * @return The cell size of the perlin noise, as supplied to the constructor.
             */
            [[nodiscard]] unsigned int GetSize() const { return m_size; }

            /**
             * Set the gradient values along one side equal to the gradient values from
             * a source perlin noise's side. Used to connect two perlin noises together
             * along their sides. The sizes of both PerlinNoises, this and the source,
             * must be the same.
             *
             * @param destSide The side to update
             * @param sourceNoise The perlin noise source for new gradients
             * @param sourceSide the size of sourceNoise from which to copy gradients
             *
             * @return True on success, false if gradients.size() is not equal to GetGridSize()
             */
            [[nodiscard]] bool SetSideGradients(Side destSide, const PerlinNoise& sourceNoise, Side sourceSide);

            /**
             * Query a subset of the perlin noise and get its values as an ImageData
             *
             * @param queryOffset The offset into the noise to query. Must be in the range [[0,0], [m_size,m_size])
             * @param querySize The size to query for data from. . Must be in the range ([0,0], [m_size,m_size])
             * @param imageSize The pixel size of image to generate
             *
             * @return The generated image from the perlin noise, or nullptr if parameters out of bounds
             */
            [[nodiscard]] Common::ImageData::Ptr GetQueryAsImage(
                const std::pair<unsigned int, unsigned int>& queryOffset,
                const unsigned int& querySize,
                const unsigned int& imageSize) const;

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

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_PERLINNOISE_H
