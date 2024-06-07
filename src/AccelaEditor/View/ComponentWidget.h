/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_COMPONENTWIDGET_H
#define ACCELAEDITOR_VIEW_COMPONENTWIDGET_H

#include <Accela/Engine/Package/Component.h>

#include <QWidget>
#include <QString>
#include <QBoxLayout>

namespace Accela
{
    class MainWindowVM;

    /**
     * Base class for component widgets. Mostly just encapsulates common logic
     * around creating a top tool section for common buttons, with subclass
     * component content underneath.
     */
    class ComponentWidget : public QWidget
    {
        Q_OBJECT

        public:

            ComponentWidget(QString title,
                            Engine::Component::Type componentType,
                            std::shared_ptr<MainWindowVM> mainVM,
                            QWidget* pParent = nullptr);

            ~ComponentWidget() override = default;

        protected:

            [[nodiscard]] QBoxLayout* CreateComponentUI();

        private slots:

            // Signals from the UI
            void UI_OnDeleteActionPressed();

        protected:

            std::shared_ptr<MainWindowVM> m_mainVM;

        private:

            QString m_title;
            Engine::Component::Type m_componentType;
    };
}

#endif //ACCELAEDITOR_VIEW_COMPONENTWIDGET_H
