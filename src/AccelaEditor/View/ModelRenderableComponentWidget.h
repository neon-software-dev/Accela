/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_MODELRENDERABLECOMPONENTWIDGET_H
#define ACCELAEDITOR_VIEW_MODELRENDERABLECOMPONENTWIDGET_H

#include "ComponentWidget.h"

#include <Accela/Engine/Package/CEntity.h>
#include <Accela/Engine/Package/CModelRenderableComponent.h>

#include <memory>

class QComboBox;

namespace Accela
{
    class MainWindowVM;

    class ModelRenderableComponentWidget : public ComponentWidget
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

            void InitUI(QBoxLayout* pContentLayout);
            void BindVM();

            void UpdateModelComboContents();

        private:

            QComboBox* m_pModelComboBox{nullptr};

            bool m_updatingModelCombo{false};
    };
}

#endif //ACCELAEDITOR_VIEW_MODELRENDERABLECOMPONENTWIDGET_H
