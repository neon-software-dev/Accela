/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_TRANSFORMCOMPONENTWIDGET_H
#define ACCELAEDITOR_VIEW_TRANSFORMCOMPONENTWIDGET_H

#include "ComponentWidget.h"

#include <Accela/Engine/Package/CEntity.h>
#include <Accela/Engine/Package/CTransformComponent.h>

#include <string>
#include <memory>

class QDoubleSpinBox;

namespace Accela
{
    class MainWindowVM;

    class TransformComponentWidget : public ComponentWidget
    {
        Q_OBJECT

        public:

            explicit TransformComponentWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent = nullptr);

            ~TransformComponentWidget() override;

        protected:

            void InitUI(QBoxLayout* pContentLayout);

        private slots:

            // Signals from the UI
            void UI_OnPositionSpinValueChanged(double);
            void UI_OnRotationSpinValueChanged(double);
            void UI_OnScaleSpinValueChanged(double);

            // Signals from the ViewModel
            void VM_OnComponentInvalidated(const Engine::CEntity::Ptr& entity, const Engine::Component::Ptr& component);

        private:

            void BindVM();

            void UpdateFieldContents();

            void UpdateComponentPositionValue();
            void UpdateComponentRotationValue();
            void UpdateComponentScaleValue();

        private:

            QDoubleSpinBox* m_pPositionXSpinBox{nullptr};
            QDoubleSpinBox* m_pPositionYSpinBox{nullptr};
            QDoubleSpinBox* m_pPositionZSpinBox{nullptr};
            QDoubleSpinBox* m_pRotationXSpinBox{nullptr};
            QDoubleSpinBox* m_pRotationYSpinBox{nullptr};
            QDoubleSpinBox* m_pRotationZSpinBox{nullptr};
            QDoubleSpinBox* m_pScaleXSpinBox{nullptr};
            QDoubleSpinBox* m_pScaleYSpinBox{nullptr};
            QDoubleSpinBox* m_pScaleZSpinBox{nullptr};

            bool m_updatingFieldContents{false};
    };
}

#endif //ACCELAEDITOR_VIEW_TRANSFORMCOMPONENTWIDGET_H
