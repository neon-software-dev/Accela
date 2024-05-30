/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MainWindowVM.h"

#include "../Util/ModelUpdate.h"

namespace Accela
{

MainWindowVM::MainWindowVM(MainWindowVM::Model model)
    : m_model(std::move(model))
{

}

void MainWindowVM::OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>& package)
{
    UpdateAndEmit(m_model.package, package, this, &MainWindowVM::VM_OnPackageChanged);
}

void MainWindowVM::OnConstructChanged(const std::optional<Engine::Construct::Ptr>& construct)
{
    UpdateAndEmit(m_model.construct, construct, this, &MainWindowVM::VM_OnConstructChanged);
}

}
