/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H
#define ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H

#include <Accela/Engine/Package/Package.h>

#include <Accela/Common/Log/ILogger.h>

#include <QObject>

#include <optional>

namespace Accela
{
    class MainWindowVM : public QObject
    {
        Q_OBJECT

        public:

            struct Model
            {
                std::optional<Engine::Package> package;
                std::optional<Engine::Construct::Ptr> construct;
                std::optional<Engine::CEntity::Ptr> entity;
            };

        public:

            MainWindowVM(Common::ILogger::Ptr logger, Model model);

            [[nodiscard]] const Model& GetModel() const noexcept { return m_model; }

            template <typename ComponentType>
            [[nodiscard]] std::optional<std::shared_ptr<ComponentType>> GetEntityComponent(const Engine::Component::Type& type);

            void OnPackageSelected(const std::optional<Engine::Package>& package);
            void OnConstructSelected(const std::optional<Engine::Construct::Ptr>& construct);
            void OnConstructInvalidated();
            void OnEntitySelected(const std::optional<Engine::CEntity::Ptr>& entity);
            void OnEntityInvalidated();
            void OnComponentInvalidated(const Engine::Component::Ptr& component);

        signals:

            void VM_OnPackageSelected(const std::optional<Engine::Package>& package);
            void VM_OnConstructSelected(const std::optional<Engine::Construct::Ptr>& construct);
            void VM_OnConstructInvalidated(const Engine::Construct::Ptr& construct);
            void VM_OnEntitySelected(const std::optional<Engine::CEntity::Ptr>& entity);
            void VM_OnEntityInvalidated(const Engine::CEntity::Ptr& entity);
            void VM_OnComponentInvalidated(const Engine::CEntity::Ptr& entity, const Engine::Component::Ptr& component);

        private:

            Common::ILogger::Ptr m_logger;
            Model m_model;
    };

    template <typename ComponentType>
    [[nodiscard]] std::optional<std::shared_ptr<ComponentType>> MainWindowVM::GetEntityComponent(const Engine::Component::Type& type)
    {
        if (!m_model.entity) { return std::nullopt; }
        const auto component = (*m_model.entity)->GetComponent(type);
        if (!component) { return std::nullopt; }
        return std::dynamic_pointer_cast<ComponentType>(*component);
    }
}

#endif //ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H
