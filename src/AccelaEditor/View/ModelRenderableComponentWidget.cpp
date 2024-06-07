/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ModelRenderableComponentWidget.h"

#include "../ViewModel/MainWindowVM.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QGroupBox>

namespace Accela
{

ModelRenderableComponentWidget::ModelRenderableComponentWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget *pParent)
    : ComponentWidget(tr("Model Renderable"), Engine::Component::Type::ModelRenderable, std::move(mainVM), pParent)
{
    InitUI(CreateComponentUI());
}

void ModelRenderableComponentWidget::InitUI(QBoxLayout* pContentLayout)
{
    m_pModelComboBox = new QComboBox();
    m_pModelComboBox->setCurrentIndex(-1);
    connect(m_pModelComboBox, &QComboBox::currentIndexChanged, this, &ModelRenderableComponentWidget::UI_OnModelComboCurrentIndexChanged);

    pContentLayout->addWidget(m_pModelComboBox);

    //
    // Initial Contents Update
    //
    UpdateModelComboContents();
}

void ModelRenderableComponentWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnComponentInvalidated, this, &ModelRenderableComponentWidget::VM_OnComponentInvalidated);
}

void ModelRenderableComponentWidget::UI_OnModelComboCurrentIndexChanged(int index)
{
    if (m_updatingModelCombo || index == -1)
    {
        return;
    }

    const auto& package = m_mainVM->GetModel().package;
    if (!package)
    {
        return;
    }

    auto modelRenderableComponent = m_mainVM->GetModel().GetEntityComponent<Engine::CModelRenderableComponent>(Engine::Component::Type::ModelRenderable);
    if (!modelRenderableComponent)
    {
        assert(false);
        return;
    }

    //

    const auto packageModelResourceNames = (*m_mainVM->GetModel().package).source->GetModelResourceNames();

    if (index >= (int)packageModelResourceNames.size())
    {
        assert(false);
        return;
    }

    const auto selectedResourceName = (*m_mainVM->GetModel().package).source->GetModelResourceNames().at(index);

    (*modelRenderableComponent)->component.modelResource = Engine::PRI(
        (*package).manifest.GetPackageName(),
        selectedResourceName
    );

    m_mainVM->OnComponentModified(*modelRenderableComponent);
}

void ModelRenderableComponentWidget::VM_OnComponentInvalidated(const Engine::CEntity::Ptr&, const Engine::Component::Ptr& component)
{
    if (component->GetType() != Engine::Component::Type::ModelRenderable)
    {
        return;
    }

    UpdateModelComboContents();
}

void ModelRenderableComponentWidget::UpdateModelComboContents()
{
    m_updatingModelCombo = true;

    //
    // Clear State
    //
    m_pModelComboBox->clear();

    //
    // Update State
    //
    const auto& package = m_mainVM->GetModel().package;
    if (!package)
    {
        m_updatingModelCombo = false;
        return;
    }

    const auto& entity = m_mainVM->GetModel().entity;
    if (!entity)
    {
        m_updatingModelCombo = false;
        return;
    }

    auto modelRenderableComponent = m_mainVM->GetModel().GetEntityComponent<Engine::CModelRenderableComponent>(Engine::Component::Type::ModelRenderable);
    if (!modelRenderableComponent)
    {
        assert(false);
        m_updatingModelCombo = false;
        return;
    }

    const auto currentModelResourceName = (*modelRenderableComponent)->component.modelResource.GetResourceName();

    int selectedIndex = -1;
    unsigned int currentIndex = 0;

    for (const auto& modelResourceName : package->source->GetModelResourceNames())
    {
        m_pModelComboBox->addItem(QString::fromStdString(modelResourceName));

        if (modelResourceName == currentModelResourceName)
        {
            selectedIndex = (int)currentIndex;
        }

        currentIndex++;
    }

    m_pModelComboBox->setCurrentIndex(selectedIndex);

    m_updatingModelCombo = false;
}

}
