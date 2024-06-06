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

EntitiesWidget::EntitiesWidget(std::shared_ptr<MainWindowVM> mainVM, SceneSyncer* pSceneEntitySyncer, QWidget *pParent)
    : QWidget(pParent)
    , m_mainVM(std::move(mainVM))
    , m_pSceneEntitySyncer(pSceneEntitySyncer)
{
    InitUI();
    BindVM();
}

EntitiesWidget::~EntitiesWidget()
{
    m_pSceneEntitySyncer = nullptr;
    m_pCreateEntityPushButton = nullptr;
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

    pActionsLayout->addWidget(m_pCreateEntityPushButton);

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
    connect(m_mainVM.get(), &MainWindowVM::VM_OnConstructSelected, this, &EntitiesWidget::VM_OnConstructSelected);
    connect(m_mainVM.get(), &MainWindowVM::VM_OnConstructInvalidated, this, &EntitiesWidget::VM_OnConstructInvalidated);
}

void EntitiesWidget::UI_OnActionCreateEntityTriggered(bool)
{
    const auto entity = std::make_shared<Engine::CEntity>(GetNewEntityName());

    m_pSceneEntitySyncer->BlockingCreateEntity(entity);

    (*m_mainVM->GetModel().construct)->AddEntity(entity);

    m_mainVM->OnConstructInvalidated();
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

    m_mainVM->OnEntitySelected(entity);
}

void EntitiesWidget::VM_OnConstructSelected(const std::optional<Engine::Construct::Ptr>&)
{
    UpdateToolbarActions();
    UpdateEntitiesListContents();
}

void EntitiesWidget::VM_OnConstructInvalidated(const Engine::Construct::Ptr&)
{
    UpdateEntitiesListContents();
}

void EntitiesWidget::UpdateToolbarActions()
{
    m_pCreateEntityPushButton->setEnabled(m_mainVM->GetModel().construct.has_value());
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

    ////

    // Now that the list is re-created, if the entity the VM thinks is selected wasn't
    // a valid option in the list, tell the VM about this
    if (m_mainVM->GetModel().entity.has_value() && selectedEntityItem == nullptr)
    {
        m_mainVM->OnEntitySelected(std::nullopt);
    }
}

std::string EntitiesWidget::GetNewEntityName() const
{
    unsigned int firstFreePostfix = 1;

    for (const auto& entity : (*m_mainVM->GetModel().construct)->GetEntities())
    {
        if (entity->name.starts_with(Engine::DEFAULT_CENTITY_NAME) &&
            entity->name.length() >= Engine::DEFAULT_CENTITY_NAME.length() + 2)
        {
            const auto postFix = entity->name.substr(Engine::DEFAULT_CENTITY_NAME.length());

            try
            {
                const auto postFixInt = std::stoi(postFix);

                firstFreePostfix = std::max((int)firstFreePostfix, postFixInt + 1);
            }
            catch (std::exception& e)
            { }
        }
    }

    return std::format("{} {}", Engine::DEFAULT_CENTITY_NAME, firstFreePostfix);
}

}
