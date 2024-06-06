/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_CONSTRUCTSWIDGET_H
#define ACCELAEDITOR_VIEW_CONSTRUCTSWIDGET_H

#include <Accela/Engine/Package/Package.h>

#include <QWidget>

class QListWidget;

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

            void UI_OnConstructsCurrentRowChanged(int index);

            void VM_OnPackageSelected(const std::optional<Engine::Package>& package);
            void VM_OnConstructSelected(const std::optional<Engine::Construct::Ptr>& construct);

        private:

            void InitUI();
            void BindVM();

            void UpdateConstructsListContents();

        private:

            std::shared_ptr<MainWindowVM> m_mainVM;

            QListWidget* m_pConstructsListWidget{nullptr};

            bool m_updatingConstructsList{false};
    };
}

#endif //ACCELAEDITOR_VIEW_CONSTRUCTSWIDGET_H
