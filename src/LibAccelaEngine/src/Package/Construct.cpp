/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Package/Construct.h>

namespace Accela::Engine
{

std::expected<Construct::Ptr, bool> Construct::FromBytes(const std::string& constructName,
                                                         const std::vector<std::byte>& data)
{
    (void)data;
    return std::make_shared<Construct>(constructName);
}

Construct::Construct(std::string name)
    : m_name(std::move(name))
{

}

std::expected<std::vector<std::byte>, bool> Construct::ToBytes() const
{
    return {};
    /*PackageModel model{
        .packageVersion = m_packageVersion
    };

    const nlohmann::json j = model;
    const auto jStr = to_string(j);

    std::vector<std::byte> byteBuffer(jStr.length());
    memcpy(byteBuffer.data(), jStr.data(), jStr.length());

    return byteBuffer;*/
}

}
