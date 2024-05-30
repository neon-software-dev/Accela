/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H
#define ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H

#include "Accela/Platform/Package/PackageSource.h"
#include <Accela/Engine/Package/Construct.h>

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
                std::optional<Platform::PackageSource::Ptr> package;
                std::optional<Engine::Construct::Ptr> construct;
            };

        public:

            explicit MainWindowVM(Model model);

            [[nodiscard]] const Model& GetModel() const noexcept { return m_model; }

            void OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>& package);
            void OnConstructChanged(const std::optional<Engine::Construct::Ptr>& construct);

        signals:

            void VM_OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>& package);
            void VM_OnConstructChanged(const std::optional<Engine::Construct::Ptr>& construct);

        private:

            Model m_model;
    };
}

#endif //ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H
