/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_UTIL_SERIALIZEOBJ_H
#define LIBACCELAENGINE_SRC_UTIL_SERIALIZEOBJ_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <nlohmann/json.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <expected>
#include <vector>
#include <cstddef>
#include <string>
#include <cstring>

namespace Accela::Engine
{
    //
    // Serialization Helpers - functions to transform objects to/from bytes, and json
    // functions to serialize common data types
    //

    /**
     * Utility function which converts a class to a corresponding model class, and then
     * serializes the model class to a vector of bytes.
     *
     * Requires [static MType MType::From(const CType&)] method to be implemented.
     *
     * Requires to_json/from_json methods for MType to be implemented.
     *
     * @tparam CType The class type to be serialized
     * @tparam MType The model class type corresponding to the class type
     * @param c The object to be serialized
     *
     * @return The model object's bytes, or false on error
     */
    template <typename CType, typename MType>
    std::expected<std::vector<std::byte>, bool> ObjectToBytes(const CType& c)
    {
        const MType model = MType::From(c);

        std::string jsonStr;

        try
        {
            const nlohmann::json j = model;
            jsonStr = j.dump(2);
        }
        catch (nlohmann::json::exception& e)
        {
            return std::unexpected(false);
        }

        std::vector<std::byte> byteBuffer(jsonStr.length());
        memcpy(byteBuffer.data(), jsonStr.data(), jsonStr.length());

        return byteBuffer;
    }

    /**
     * Utility function which creates an object from a model class represented as bytes.
     *
     * Requires [CType MType::From()] method to be implemented.
     *
     * Requires to_json/from_json methods for MType to be implemented.
     *
     * @tparam CType The class type to be deserialized
     * @tparam MType The model class type corresponding to the class type
     * @param bytes The bytes of an MType object
     *
     * @return The deserialized CType class, or false on error
     */
    template <typename CType, typename MType>
    std::expected<CType, bool> ObjectFromBytes(const std::vector<std::byte>& bytes)
    {
        MType model{};

        try
        {
            const nlohmann::json j = nlohmann::json::parse(bytes.cbegin(), bytes.cend());
            model = j.template get<MType>();
        }
        catch (nlohmann::json::exception& e)
        {
            return std::unexpected(false);
        }

        return model.From();
    }

    void to_json(nlohmann::json& j, const ResourceIdentifier& m);
    void from_json(const nlohmann::json& j, ResourceIdentifier& m);
}

namespace glm
{
    void to_json(nlohmann::json& j, const vec3& m);
    void from_json(const nlohmann::json& j, vec3& m);

    void to_json(nlohmann::json& j, const vec4& m);
    void from_json(const nlohmann::json& j, vec4& m);

    void to_json(nlohmann::json& j, const quat& m);
    void from_json(const nlohmann::json& j, quat& m);
}

#endif //LIBACCELAENGINE_SRC_UTIL_SERIALIZEOBJ_H
