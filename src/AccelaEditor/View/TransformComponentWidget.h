/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_TRANSFORMCOMPONENTWIDGET_H
#define ACCELAEDITOR_VIEW_TRANSFORMCOMPONENTWIDGET_H

#include <Accela/Engine/Package/CEntity.h>
#include <Accela/Engine/Package/CTransformComponent.h>

#include <QWidget>

#include <string>
#include <memory>

class QDoubleSpinBox;

namespace Accela
{
    class MainWindowVM;

    class TransformComponentWidget : public QWidget
    {
        Q_OBJECT

        public:

            explicit TransformComponentWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent = nullptr);

            ~TransformComponentWidget() override;

        private slots:

            // Signals from the UI
            void UI_OnPositionXSpinValueChanged(double d);
            void UI_OnPositionYSpinValueChanged(double d);
            void UI_OnPositionZSpinValueChanged(double d);

            // Signals from the ViewModel
            void VM_OnComponentInvalidated(const Engine::CEntity::Ptr& entity, const Engine::Component::Ptr& component);

        private:

            void InitUI();
            void BindVM();

            void UpdateFieldContents();

        private:

            std::shared_ptr<MainWindowVM> m_mainVM;

            QDoubleSpinBox* m_pPositionXSpinBox{nullptr};
            QDoubleSpinBox* m_pPositionYSpinBox{nullptr};
            QDoubleSpinBox* m_pPositionZSpinBox{nullptr};

            bool m_updatingFieldContents{false};
    };
}

#endif //ACCELAEDITOR_VIEW_TRANSFORMCOMPONENTWIDGET_H
