/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EntitiesWidget.h"
#include "../EditorScene/SceneSyncer.h"

#include <QToolBar>
#include <QBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>

#include <format>

namespace Accela
{

EntitiesWidget::EntitiesWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget *pParent)
    : QWidget(pParent)
    , m_mainVM(std::move(mainVM))
{
    InitUI();
    BindVM();
}

EntitiesWidget::~EntitiesWidget()
{
    m_pCreateEntityPushButton = nullptr;
    m_pDeleteEntityPushButton = nullptr;
    m_pEntitiesListWidget = nullptr;
}

void EntitiesWidget::InitUI()
{
    //
    // Top/Actions Toolbar
    //
    auto pActionsLayout = new QHBoxLayout();
    pActionsLayout->setAlignment(Qt::AlignmentFlag::AlignLeft);

    m_pCreateEntityPushButton = new QPushButton();
    m_pCreateEntityPushButton->setIcon(QIcon(":/icons/add.png"));
    connect(m_pCreateEntityPushButton, &QPushButton::clicked, this, &EntitiesWidget::UI_OnActionCreateEntityTriggered);

    m_pDeleteEntityPushButton = new QPushButton();
    m_pDeleteEntityPushButton->setIcon(QIcon(":/icons/delete.png"));
    connect(m_pDeleteEntityPushButton, &QPushButton::clicked, this, &EntitiesWidget::UI_OnActionDeleteEntityTriggered);

    pActionsLayout->addWidget(m_pCreateEntityPushButton);
    pActionsLayout->addWidget(m_pDeleteEntityPushButton);

    //
    // Entities List Widget
    //
    m_pEntitiesListWidget = new QListWidget();
    connect(m_pEntitiesListWidget, &QListWidget::currentRowChanged, this, &EntitiesWidget::UI_OnEntityListCurrentRowChanged);

    //
    // Main Layout
    //
    auto pLayout = new QVBoxLayout(this);
    pLayout->addLayout(pActionsLayout);
    pLayout->addWidget(m_pEntitiesListWidget, 1);

    //
    // Initial contents update
    //
    UpdateToolbarActions();
    UpdateEntitiesListContents();
}

void EntitiesWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnConstructChanged, this, &EntitiesWidget::VM_OnConstructChanged);
    connect(m_mainVM.get(), &MainWindowVM::VM_OnConstructInvalidated, this, &EntitiesWidget::VM_OnConstructInvalidated);
    connect(m_mainVM.get(), &MainWindowVM::VM_OnEntityChanged, this, &EntitiesWidget::VM_OnEntityChanged);
}

void EntitiesWidget::UI_OnActionCreateEntityTriggered(bool)
{
    m_mainVM->OnCreateEntity();
}

void EntitiesWidget::UI_OnActionDeleteEntityTriggered(bool)
{
    m_mainVM->OnDeleteEntity();
}

void EntitiesWidget::UI_OnEntityListCurrentRowChanged(int currentRow)
{
    // If we're updating the entities list, which involves clearing it and then recreating
    // its items, ignore UI events about the current item being changed
    if (m_updatingEntitiesList)
    {
        return;
    }

    std::optional<Engine::CEntity::Ptr> entity;

    const auto& constructEntities = (*m_mainVM->GetModel().construct)->GetEntities();

    if (currentRow >= 0 && (unsigned int)currentRow < constructEntities.size())
    {
        entity = constructEntities.at(currentRow);
    }

    if (entity.has_value())
    {
        m_mainVM->OnLoadEntity((*entity)->name);
    }
}

void EntitiesWidget::VM_OnConstructChanged(const std::optional<Engine::Construct::Ptr>&)
{
    UpdateToolbarActions();
    UpdateEntitiesListContents();
}

void EntitiesWidget::VM_OnConstructInvalidated(const Engine::Construct::Ptr&)
{
    UpdateEntitiesListContents();
}

void EntitiesWidget::VM_OnEntityChanged(const std::optional<Engine::CEntity::Ptr>&)
{
    UpdateToolbarActions();
}

void EntitiesWidget::UpdateToolbarActions()
{
    m_pCreateEntityPushButton->setEnabled(m_mainVM->GetModel().construct.has_value());
    m_pDeleteEntityPushButton->setEnabled(m_mainVM->GetModel().entity.has_value());
}

void EntitiesWidget::UpdateEntitiesListContents()
{
    m_updatingEntitiesList = true;

    //
    // Clear State
    //
    m_pEntitiesListWidget->clear();

    //
    // Update State
    //
    const auto& construct = m_mainVM->GetModel().construct;
    if (!construct)
    {
        m_updatingEntitiesList = false;
        return;
    }

    QListWidgetItem* selectedEntityItem = nullptr;

    for (const auto& entity : (*construct)->GetEntities())
    {
        const auto entityItem = new QListWidgetItem(QString::fromStdString(entity->name), m_pEntitiesListWidget);

        m_pEntitiesListWidget->addItem(entityItem);

        if (entity == m_mainVM->GetModel().entity)
        {
            selectedEntityItem = entityItem;
        }
    }

    if (selectedEntityItem != nullptr)
    {
        m_pEntitiesListWidget->setCurrentItem(selectedEntityItem);
    }

    m_updatingEntitiesList = false;
}

}
