/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_ENTITYWIDGET_H
#define ACCELAEDITOR_VIEW_ENTITYWIDGET_H

#include "../ViewModel/MainWindowVM.h"

#include <QWidget>

class QToolButton;
class QLayout;
class QAction;

namespace Accela
{
    class EntityWidget : public QWidget
    {
        Q_OBJECT

        public:

            explicit EntityWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent = nullptr);
            ~EntityWidget() override;

        private slots:

            // Signals from the UI
            void UI_OnAddComponentActionTriggered(QAction *action);

            // Signals from the ViewModel
            void VM_OnEntitySelected(const std::optional<Engine::CEntity::Ptr>& entity);
            void VM_OnEntityInvalidated(const Engine::CEntity::Ptr& entity);

        private:

            void InitUI();
            void BindVM();

            void UpdateToolbarActions();
            void UpdateComponentsListContents();

        private:

            std::shared_ptr<MainWindowVM> m_mainVM;

            QToolButton* m_pAddComponentToolButton{nullptr};
            QAction* m_pAddTransformComponentAction{nullptr};
            QAction* m_pAddModelRenderableComponentAction{nullptr};

            QLayout* m_pComponentsLayout{nullptr};
    };
}

#endif //ACCELAEDITOR_VIEW_ENTITYWIDGET_H
