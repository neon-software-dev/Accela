/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_CONSTRUCTSWIDGET_H
#define ACCELAEDITOR_VIEW_CONSTRUCTSWIDGET_H

#include "Accela/Platform/Package/PackageSource.h"
#include <Accela/Engine/Package/Construct.h>

#include <QWidget>

class QListWidget;
class QComboBox;

namespace Accela
{
    class MainWindowVM;

    class ConstructsWidget : public QWidget
    {
        Q_OBJECT

        public:

            explicit ConstructsWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent = nullptr);
            ~ConstructsWidget() override;

        private slots:

            void UI_OnConstructComboCurrentIndexChanged(int index);

            void VM_OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>& package);
            void VM_OnConstructChanged(const std::optional<Engine::Construct::Ptr>& construct);

        private:

            void InitUI();
            void BindVM();

            void UpdateConstructsComboBoxContents();
            void UpdateEntitiesListContents();

        private:

            std::shared_ptr<MainWindowVM> m_mainVM;

            QComboBox* m_pConstructComboBox{nullptr};
            QListWidget* m_pEntitiesListWidget{nullptr};
    };
}

#endif //ACCELAEDITOR_VIEW_CONSTRUCTSWIDGET_H
