/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_ENTITIESWIDGET_H
#define ACCELAEDITOR_VIEW_ENTITIESWIDGET_H

#include "../ViewModel/MainWindowVM.h"

#include <Accela/Engine/Package/Construct.h>

#include <QWidget>

class QListWidget;
class QPushButton;

namespace Accela
{
    class SceneSyncer;

    class EntitiesWidget : public QWidget
    {
        Q_OBJECT

        public:

            explicit EntitiesWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent = nullptr);
            ~EntitiesWidget() override;

        private slots:

            // Signals from the UI
            void UI_OnActionCreateEntityTriggered(bool);
            void UI_OnActionDeleteEntityTriggered(bool);
            void UI_OnEntityListCurrentRowChanged(int);

            // Signals from the ViewModel
            void VM_OnConstructChanged(const std::optional<Engine::Construct::Ptr>& construct);
            void VM_OnConstructInvalidated(const Engine::Construct::Ptr& construct);
            void VM_OnEntityChanged(const std::optional<Engine::CEntity::Ptr>& entity);

        private:

            void InitUI();
            void BindVM();

            void UpdateToolbarActions();
            void UpdateEntitiesListContents();

        private:

            std::shared_ptr<MainWindowVM> m_mainVM;

            QPushButton* m_pCreateEntityPushButton{nullptr};
            QPushButton* m_pDeleteEntityPushButton{nullptr};
            QListWidget* m_pEntitiesListWidget{nullptr};

            bool m_updatingEntitiesList{false};
    };
}

#endif //ACCELAEDITOR_VIEW_ENTITIESWIDGET_H
