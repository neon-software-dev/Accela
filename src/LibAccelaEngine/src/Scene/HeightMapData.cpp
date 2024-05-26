#include "HeightMapData.h"

#include <Accela/Render/Mesh/StaticMesh.h>

#include <vector>
#include <cstddef>

namespace Accela::Engine
{

// Maps a value X in the range of [A1...A2] into the range [B1...B2]
template<typename T>
inline T MapValue(const std::pair<T,T>& a, const std::pair<T, T>& b, const T& val)
{
    return b.first + (((float)(val - a.first) / (float)(a.second - a.first)) * (b.second - b.first));
}

std::vector<double> GenerateHeightMapDataValues(const Common::ImageData::Ptr& heightMapImage,
                                                const Render::USize& heightMapDataSize,
                                                const float& displacementFactor)
{
    std::vector<double> data;
    data.reserve(heightMapDataSize.w * heightMapDataSize.h);

    for (std::size_t y = 0; y < heightMapDataSize.h; ++y)
    {
        for (std::size_t x = 0; x < heightMapDataSize.w; ++x)
        {
            // Map from data/grid position within the height map to pixel position within the image
            const auto imageXPixel = MapValue({0, heightMapDataSize.w - 1}, {0, heightMapImage->GetPixelWidth() - 1}, x);
            const auto imageYPixel = MapValue({0, heightMapDataSize.h - 1}, {0, heightMapImage->GetPixelHeight() - 1}, y);

            const auto imagePixelIndex = (heightMapImage->GetPixelWidth() * imageYPixel) + imageXPixel;
            const auto imagePixelBytes = heightMapImage->GetPixelBytes(0, imagePixelIndex);

            const std::byte& pixelValue = imagePixelBytes[0]; // Noteworthy, assuming grayscale heightmap, only looking at first byte

            data.push_back(
                (((double)std::to_integer<unsigned char>(pixelValue)) / 255.0f)
                * displacementFactor
            );
        }
    }

    return data;
}

HeightMapData::Ptr GenerateHeightMapData(const Common::ImageData::Ptr& heightMapImage,
                                         const Render::USize& heightMapDataSize,
                                         const Render::USize& meshSize_worldSpace,
                                         const float& displacementFactor)
{
    //
    // Create height map data values from sampling the image's data
    //
    const auto heightMapDataValues = GenerateHeightMapDataValues(heightMapImage, heightMapDataSize, displacementFactor);

    //
    // Determine min/max height map values
    //
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::min();

    for (const auto& val : heightMapDataValues)
    {
        minValue = std::fmin(minValue, val);
        maxValue = std::fmax(maxValue, val);
    }

    //
    // Result
    //
    HeightMapData::Ptr heightMapData = std::make_shared<HeightMapData>();
    heightMapData->data = heightMapDataValues;
    heightMapData->dataSize = heightMapDataSize;
    heightMapData->minValue = minValue;
    heightMapData->maxValue = maxValue;
    heightMapData->meshSize_worldSpace = meshSize_worldSpace;

    return heightMapData;
}

Render::Mesh::Ptr GenerateHeightMapMesh(const Render::MeshId& id,
                                        const HeightMapData& heightMapData,
                                        const Render::USize& meshSize_worldSpace,
                                        const std::string& tag)
{
    std::vector<Render::MeshVertex> vertices;
    vertices.reserve(heightMapData.dataSize.w * heightMapData.dataSize.h);

    std::vector<uint32_t> indices;
    indices.reserve(heightMapData.dataSize.w * heightMapData.dataSize.h);

    // World distance between adjacent vertices in x and z directions
    const float vertexXDelta = (float)meshSize_worldSpace.w / (float)(heightMapData.dataSize.w - 1);
    const float vertexZDelta = (float)meshSize_worldSpace.h / (float)(heightMapData.dataSize.h - 1);

    // Current world position of the vertex we're processing. Start at the back left corner of the mesh.
    float xPos = -1.0f * (float)meshSize_worldSpace.w / 2.0f;
    float zPos = 1.0f * (float)meshSize_worldSpace.h / 2.0f;

    // Loop over data points in the height map and create a vertex for each
    for (std::size_t y = 0; y < heightMapData.dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < heightMapData.dataSize.w; ++x)
        {
            // The height map data is stored with the "top" row of the height map image in the start of the
            // vector. As we're building our vertices starting from the bottom left, flip the Y coordinate so
            // the bottom left vertex is getting its data from the end of the vector, where the bottom height map
            // row is.
            const auto flippedY = (heightMapData.dataSize.h - 1) - y;

            // Index of this vertex's height map data entry
            const auto dataIndex = x + (flippedY * heightMapData.dataSize.w);

            const auto position = glm::vec3(xPos, heightMapData.data[dataIndex], zPos);

            // Normals are determined in a separate loop below, once we have all the positions of each vertex figured out
            const auto normal = glm::vec3(0,1,0);

            const float uvX = (float)x / ((float)heightMapData.dataSize.w - 1);
            const float uvY = (float)flippedY / ((float)heightMapData.dataSize.h - 1);
            const auto uv = glm::vec2(uvX, uvY);

            const auto tangent = glm::vec3(0,1,0); // TODO: Can/should we calculate this manually

            vertices.emplace_back(position, normal, uv, tangent);

            xPos += vertexXDelta;
        }

        xPos = -1.0f * (float)meshSize_worldSpace.w / 2.0f;
        zPos -= vertexZDelta;
    }

    // Loop over vertices and calculate normals from looking at neighboring vertices
    for (std::size_t y = 0; y < heightMapData.dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < heightMapData.dataSize.w; ++x)
        {
            // Index of this vertex's height map data entry
            const auto dataIndex = x + (y * heightMapData.dataSize.w);

            // model-space position of the vertex to compute a normal for
            const auto centerPosition = vertices[dataIndex].position;

            const bool isLeftEdgeVertex = x == 0;
            const bool isRightEdgeVertex = x == (heightMapData.dataSize.w - 1);
            const bool isBottomEdgeVertex = y == 0;
            const bool isTopEdgeVertex = y == (heightMapData.dataSize.h - 1);

            // Get the positions of the vertices to all four sides of this vertex. If some don't exist
            // because the vertex is on an edge, just default them to the center vertex's position.
            glm::vec3 leftVertexPosition = centerPosition;
            glm::vec3 rightVertexPosition = centerPosition;
            glm::vec3 upVertexPosition = centerPosition;
            glm::vec3 bottomVertexPosition = centerPosition;

            if (!isLeftEdgeVertex)
            {
                leftVertexPosition = vertices[dataIndex - 1].position;
            }
            if (!isBottomEdgeVertex)
            {
                bottomVertexPosition = vertices[dataIndex - heightMapData.dataSize.w].position;
            }
            if (!isRightEdgeVertex)
            {
                rightVertexPosition = vertices[dataIndex + 1].position;
            }
            if (!isTopEdgeVertex)
            {
                upVertexPosition = vertices[dataIndex + heightMapData.dataSize.w].position;
            }

            // Calculate vectors that point left to right and back to front across the center vertex
            const glm::vec3 dx = rightVertexPosition - leftVertexPosition;
            const glm::vec3 dz = bottomVertexPosition - upVertexPosition;

            // Center vertex normal is the normalized cross product of these vectors
            vertices[dataIndex].normal = glm::normalize(glm::cross(dz, dx));
        }
    }

    // Loop over data points in the height map (minus one row/column) and create indices
    for (std::size_t y = 0; y < heightMapData.dataSize.h - 1; ++y)
    {
        for (std::size_t x = 0; x < heightMapData.dataSize.w - 1; ++x)
        {
            const auto dataIndex = x + (y * heightMapData.dataSize.w);

            // triangle 1
            indices.push_back(dataIndex);
            indices.push_back(dataIndex + 1);
            indices.push_back(dataIndex + heightMapData.dataSize.w);

            // triangle 2
            indices.push_back(dataIndex + 1);
            indices.push_back(dataIndex + heightMapData.dataSize.w + 1);
            indices.push_back(dataIndex + heightMapData.dataSize.w);
        }
    }

    return std::make_shared<Render::StaticMesh>(id, vertices, indices, tag);
}

}
