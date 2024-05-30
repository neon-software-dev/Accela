/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_CREATEPACKAGEDIALOG_H
#define ACCELAEDITOR_VIEW_CREATEPACKAGEDIALOG_H

#include <QDialog>

#include <memory>
#include <filesystem>

class QLineEdit;
class QPushButton;

namespace Accela
{
    class PackageSelectVM;

    class CreatePackageDialog : public QDialog
    {
        Q_OBJECT

        public:

            explicit CreatePackageDialog(QWidget* pParent = nullptr);

            [[nodiscard]] std::optional<std::filesystem::path> GetResult() const { return m_result; }

        private slots:

            void OnCreateDirectoryButtonClicked(bool);
            void OnCreateButtonClicked(bool);

            void UI_OnCreateNameChanged(const QString& text);
            void UI_OnCreateDirectoryChanged(const QString& text);

            void VM_OnCreateNameChanged(const std::optional<std::string>& createName);
            void VM_OnCreateDirectoryChanged(const std::optional<std::filesystem::path>& createDirectory);
            void VM_OnCreateActionValidChanged(const bool& createActionValid);

        private:

            void InitUI();
            void BindVM();

            static void DisplayErrorMessage(const std::string& errorMsg);

        private:

            std::unique_ptr<PackageSelectVM> m_vm;

            QLineEdit* m_pCreateNameEdit{nullptr};
            QLineEdit* m_pCreateDirectoryEdit{nullptr};

            QPushButton* m_pCreateButton{nullptr};

            std::optional<std::filesystem::path> m_result;
    };
}

#endif //ACCELAEDITOR_VIEW_CREATEPACKAGEDIALOG_H
