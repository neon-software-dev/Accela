/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PackageSelectVM.h"

#include "../Util/ModelUpdate.h"

namespace Accela
{

void PackageSelectVM::OnCreateNameChanged(const std::string& createName)
{
    std::optional<std::string> createNameOpt;
    if (!createName.empty())
    {
        createNameOpt = createName;
    }

    UpdateAndEmit(m_model.createName, createNameOpt, this, &PackageSelectVM::VM_OnCreateNameChanged);
    UpdateAndEmit(m_model.createActionValid, IsCreateActionValid(), this, &PackageSelectVM::VM_OnCreateActionValidChanged);
}

void PackageSelectVM::OnCreateDirectoryChanged(const std::string& createDirectory)
{
    std::optional<std::filesystem::path> createDirectoryOpt;
    if (!createDirectory.empty())
    {
        createDirectoryOpt = std::filesystem::path(createDirectory);
    }

    UpdateAndEmit(m_model.createDirectory, createDirectoryOpt, this, &PackageSelectVM::VM_OnCreateDirectoryChanged);
    UpdateAndEmit(m_model.createActionValid, IsCreateActionValid(), this, &PackageSelectVM::VM_OnCreateActionValidChanged);
}

bool PackageSelectVM::IsCreateActionValid() const
{
    return m_model.createName.has_value() && m_model.createDirectory.has_value();
}

}
