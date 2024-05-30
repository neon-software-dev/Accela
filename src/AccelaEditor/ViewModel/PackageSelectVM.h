/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEWMODEL_PACKAGESELECTVM_H
#define ACCELAEDITOR_VIEWMODEL_PACKAGESELECTVM_H

#include <QObject>

#include <optional>
#include <filesystem>

namespace Accela
{
    class PackageSelectVM : public QObject
    {
        Q_OBJECT

        public:

            struct Model
            {
                std::optional<std::string> createName;
                std::optional<std::filesystem::path> createDirectory;

                bool createActionValid{false};
            };

        public:

            [[nodiscard]] const Model& GetModel() const noexcept { return m_model; }

            void OnCreateNameChanged(const std::string& createName);
            void OnCreateDirectoryChanged(const std::string& createDirectory);

        signals:

            void VM_OnCreateNameChanged(const std::optional<std::string>& createName);
            void VM_OnCreateDirectoryChanged(const std::optional<std::filesystem::path>& createDirectory);
            void VM_OnCreateActionValidChanged(const bool& createActionValid);

        private:

            [[nodiscard]] bool IsCreateActionValid() const;

        private:

            Model m_model;
    };
}

#endif //ACCELAEDITOR_VIEWMODEL_PACKAGESELECTVM_H
