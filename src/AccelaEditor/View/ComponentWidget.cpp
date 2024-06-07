/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ComponentWidget.h"

#include "../ViewModel/MainWindowVM.h"

#include <QGroupBox>
#include <QBoxLayout>
#include <QPushButton>

namespace Accela
{

ComponentWidget::ComponentWidget(QString title,
                                 Engine::Component::Type componentType,
                                 std::shared_ptr<MainWindowVM> mainVM,
                                 QWidget *pParent)
    : QWidget(pParent)
    , m_mainVM(std::move(mainVM))
    , m_title(std::move(title))
    , m_componentType(componentType)
{

}

QBoxLayout* ComponentWidget::CreateComponentUI()
{
    auto pGroupBox = new QGroupBox(m_title, this);
    auto pGroupBoxLayout = new QVBoxLayout(pGroupBox);

    //
    // Tools layout
    //
    auto pToolsLayout = new QHBoxLayout();
    pToolsLayout->setAlignment(Qt::AlignmentFlag::AlignRight);

    auto pDeletePushButton = new QPushButton();
    pDeletePushButton->setIcon(QIcon(":/icons/delete.png"));
    connect(pDeletePushButton, &QPushButton::pressed, this, &ComponentWidget::UI_OnDeleteActionPressed);

    pToolsLayout->addWidget(pDeletePushButton);

    pGroupBoxLayout->addLayout(pToolsLayout);

    //
    // Main layout
    //
    auto pLayout = new QVBoxLayout(this);
    pLayout->addWidget(pGroupBox);

    return pGroupBoxLayout;
}

void ComponentWidget::UI_OnDeleteActionPressed()
{
    // TODO!
}

}
