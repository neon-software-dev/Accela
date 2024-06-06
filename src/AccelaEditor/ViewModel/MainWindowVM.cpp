/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MainWindowVM.h"

#include "../Util/ModelUpdate.h"

namespace Accela
{

MainWindowVM::MainWindowVM(Common::ILogger::Ptr logger, MainWindowVM::Model model)
    : m_logger(std::move(logger))
    , m_model(std::move(model))
{

}

void MainWindowVM::OnPackageSelected(const std::optional<Engine::Package>& package)
{
    if (UpdateAndEmit(m_model.package, package, this, &MainWindowVM::VM_OnPackageSelected))
    {
        const auto packageStr = package.has_value() ? package->manifest.GetPackageName() : "None";
        m_logger->Log(Common::LogLevel::Info, "MainWindowVM::OnPackageSelected: {}", packageStr);

        // When selected package changed, reset selected construct
        OnConstructSelected(std::nullopt);
    }
}

void MainWindowVM::OnConstructSelected(const std::optional<Engine::Construct::Ptr>& construct)
{
    if (UpdateAndEmit(m_model.construct, construct, this, &MainWindowVM::VM_OnConstructSelected))
    {
        const auto constructStr = construct.has_value() ? (*construct)->GetName() : "None";
        m_logger->Log(Common::LogLevel::Info, "MainWindowVM::OnConstructSelected: {}", constructStr);

        // When selected construct changed, reset selected entity
        OnEntitySelected(std::nullopt);
    }
}

void MainWindowVM::OnConstructInvalidated()
{
    if (!m_model.construct.has_value())
    {
        assert(false);
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "MainWindowVM::OnConstructInvalidated: {}", (*m_model.construct)->GetName());
    emit VM_OnConstructInvalidated(*m_model.construct);
}

void MainWindowVM::OnEntitySelected(const std::optional<Engine::CEntity::Ptr>& entity)
{
    if (UpdateAndEmit(m_model.entity, entity, this, &MainWindowVM::VM_OnEntitySelected))
    {
        const auto entityStr = entity.has_value() ? (*entity)->name : "None";
        m_logger->Log(Common::LogLevel::Info, "MainWindowVM::OnEntitySelected: {}", entityStr);
    }
}

void MainWindowVM::OnEntityInvalidated()
{
    if (!m_model.entity.has_value())
    {
        assert(false);
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "MainWindowVM::OnEntityInvalidated: {}", (*m_model.entity)->name);
    emit VM_OnEntityInvalidated(*m_model.entity);
}

void MainWindowVM::OnComponentInvalidated(const Engine::Component::Ptr& component)
{
    if (!m_model.entity.has_value())
    {
        assert(false);
        return;
    }

    m_logger->Log(Common::LogLevel::Info,
      "MainWindowVM::OnComponentInvalidated: {}, {}", (*m_model.entity)->name, (unsigned int)component->GetType());
    emit VM_OnComponentInvalidated(*m_model.entity, component);
}

}
