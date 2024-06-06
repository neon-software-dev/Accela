/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_MODELRENDERABLECOMPONENTWIDGET_H
#define ACCELAEDITOR_VIEW_MODELRENDERABLECOMPONENTWIDGET_H

#include <Accela/Engine/Package/CEntity.h>
#include <Accela/Engine/Package/CModelRenderableComponent.h>

#include <QWidget>

#include <memory>

class QComboBox;

namespace Accela
{
    class MainWindowVM;

    class ModelRenderableComponentWidget : public QWidget
    {
        Q_OBJECT

        public:

            explicit ModelRenderableComponentWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent = nullptr);

        private slots:

            // Signals from the UI
            void UI_OnModelComboCurrentIndexChanged(int index);

            // Signals from the ViewModel
            void VM_OnComponentInvalidated(const Engine::CEntity::Ptr& entity, const Engine::Component::Ptr& component);

        private:

            void InitUI();
            void BindVM();

            void UpdateModelComboContents();

        private:

            std::shared_ptr<MainWindowVM> m_mainVM;

            QComboBox* m_pModelComboBox{nullptr};

            bool m_updatingModelCombo{false};
    };
}

#endif //ACCELAEDITOR_VIEW_MODELRENDERABLECOMPONENTWIDGET_H
