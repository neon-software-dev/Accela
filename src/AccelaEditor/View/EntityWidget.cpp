/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EntityWidget.h"
#include "MinHeightLayout.h"
#include "TransformComponentWidget.h"
#include "ModelRenderableComponentWidget.h"

#include <Accela/Engine/Package/CTransformComponent.h>
#include <Accela/Engine/Package/CModelRenderableComponent.h>

#include <QBoxLayout>
#include <QListWidget>
#include <QAction>
#include <QScrollArea>
#include <QMenu>
#include <QToolButton>

namespace Accela
{

EntityWidget::EntityWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget *pParent)
    : QWidget(pParent)
    , m_mainVM(std::move(mainVM))
{
    InitUI();
    BindVM();
}

EntityWidget::~EntityWidget()
{
    m_pAddComponentToolButton = nullptr;
    m_pAddTransformComponentAction = nullptr;
    m_pAddModelRenderableComponentAction = nullptr;
    m_pComponentsLayout = nullptr;
}

void EntityWidget::InitUI()
{
    //
    // Top/Actions Toolbar
    //
    auto pActionsLayout = new QHBoxLayout();
    pActionsLayout->setAlignment(Qt::AlignmentFlag::AlignLeft);

    m_pAddTransformComponentAction = new QAction(tr("Transform"));
    m_pAddModelRenderableComponentAction = new QAction(tr("Model Renderable"));

    auto *pMenu = new QMenu();
    pMenu->addAction(m_pAddTransformComponentAction);
    pMenu->addAction(m_pAddModelRenderableComponentAction);

    m_pAddComponentToolButton = new QToolButton();
    m_pAddComponentToolButton->setIcon(QIcon(":/icons/add.png"));
    m_pAddComponentToolButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    m_pAddComponentToolButton->setMenu(pMenu);
    connect(m_pAddComponentToolButton, &QToolButton::triggered, this, &EntityWidget::UI_OnAddComponentActionTriggered);

    pActionsLayout->addWidget(m_pAddComponentToolButton);

    //
    // Component Widgets Scroll Area+Layout
    //
    auto pComponentsScrollArea = new QScrollArea();
    pComponentsScrollArea->setWidgetResizable(true);

    auto pScrolledWidget = new QWidget();
    m_pComponentsLayout = new MinHeightLayout(pScrolledWidget);

    pComponentsScrollArea->setWidget(pScrolledWidget);

    //
    // Main Layout
    //
    auto pLayout = new QVBoxLayout(this);
    pLayout->addLayout(pActionsLayout);
    pLayout->addWidget(pComponentsScrollArea, 1);

    //
    // Initial contents update
    //
    UpdateToolbarActions();
    UpdateComponentsListContents();
}

void EntityWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnEntitySelected, this, &EntityWidget::VM_OnEntitySelected);
    connect(m_mainVM.get(), &MainWindowVM::VM_OnEntityInvalidated, this, &EntityWidget::VM_OnEntityInvalidated);
}

void EntityWidget::UI_OnAddComponentActionTriggered(QAction *pAction)
{
    Engine::Component::Ptr component;

    if (pAction == m_pAddTransformComponentAction)
    {
        component = std::make_shared<Engine::CTransformComponent>();
        (*m_mainVM->GetModel().entity)->components.push_back(component);
    }
    else if (pAction == m_pAddModelRenderableComponentAction)
    {
        component = std::make_shared<Engine::CModelRenderableComponent>();
        (*m_mainVM->GetModel().entity)->components.push_back(component);
    }
    else
    {
        assert(false);
        return;
    }

    m_mainVM->OnEntityInvalidated();
    m_mainVM->OnComponentInvalidated(component);
}

void EntityWidget::VM_OnEntitySelected(const std::optional<Engine::CEntity::Ptr>&)
{
    UpdateToolbarActions();
    UpdateComponentsListContents();
}

void EntityWidget::VM_OnEntityInvalidated(const Engine::CEntity::Ptr&)
{
    UpdateToolbarActions();
    UpdateComponentsListContents();
}

void EntityWidget::UpdateToolbarActions()
{
    const auto& entity = m_mainVM->GetModel().entity;

    m_pAddComponentToolButton->setEnabled(entity.has_value());
    m_pAddTransformComponentAction->setEnabled(entity.has_value() && !(*entity)->GetComponent(Engine::Component::Type::Transform).has_value());
    m_pAddModelRenderableComponentAction->setEnabled(entity.has_value() && !(*entity)->GetComponent(Engine::Component::Type::ModelRenderable).has_value());
}

void EntityWidget::UpdateComponentsListContents()
{
    //
    // Clear State
    //
    while (auto pLayoutItem = m_pComponentsLayout->takeAt(0))
    {
        delete pLayoutItem->widget();
    }

    //
    // Update State
    //
    const auto& entity = m_mainVM->GetModel().entity;
    if (!entity)
    {
        return;
    }

    for (const auto& component : (*entity)->components)
    {
        switch (component->GetType())
        {
            case Engine::Component::Type::Transform:
                m_pComponentsLayout->addWidget(new TransformComponentWidget(m_mainVM));
            break;
            case Engine::Component::Type::ModelRenderable:
                m_pComponentsLayout->addWidget(new ModelRenderableComponentWidget(m_mainVM));
            break;
        }
    }
}

}
