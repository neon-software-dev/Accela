/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Package/Construct.h>

#include "ConstructModel.h"

#include "../Util/SerializeObj.h"

namespace Accela::Engine
{

Construct::Construct(std::string name)
    : m_name(std::move(name))
{

}

std::expected<Construct::Ptr, bool> Construct::FromBytes(const std::vector<std::byte>& data)
{
    const auto constructExpect = ObjectFromBytes<Construct, ConstructModel>(data);
    if (!constructExpect)
    {
        return std::unexpected(false);
    }

    return std::make_shared<Construct>(*constructExpect);
}

std::expected<std::vector<std::byte>, bool> Construct::ToBytes() const
{
    return ObjectToBytes<Construct, ConstructModel>(*this);
}

void Construct::AddEntity(const CEntity::Ptr& entity)
{
    m_entities.push_back(entity);
}

void Construct::RemoveEntity(const std::string& entityName)
{
    std::erase_if(m_entities, [&](const auto& entity){
        return entity->name == entityName;
    });
}

}
