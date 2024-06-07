/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_RESOURCESWIDGET_H
#define ACCELAEDITOR_VIEW_RESOURCESWIDGET_H

#include <Accela/Engine/Package/Package.h>

#include <QWidget>

#include <memory>

class QListWidget;
class QComboBox;

namespace Accela
{
    class MainWindowVM;

    class ResourcesWidget : public QWidget
    {
        Q_OBJECT

        public:

            explicit ResourcesWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent = nullptr);
            ~ResourcesWidget() override;

        private slots:

            // Signals from the UI
            void UI_TypeComboCurrentIndexChanged(int index);

            // Signals from the ViewModel
            void VM_OnPackageChanged(const std::optional<Engine::Package>& package);

        private:

            void InitUI();
            void BindVM();

            void UpdateResourcesListContents();

        private:

            std::shared_ptr<MainWindowVM> m_mainVM;

            QComboBox* m_pTypeComboBox{nullptr};
            QListWidget* m_pResourcesListWidget{nullptr};
    };
}

#endif //ACCELAEDITOR_VIEW_RESOURCESWIDGET_H
