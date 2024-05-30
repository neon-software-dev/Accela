/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ConstructsWidget.h"

#include "../ViewModel/MainWindowVM.h"

#include <QListWidget>
#include <QBoxLayout>
#include <QComboBox>

namespace Accela
{

ConstructsWidget::ConstructsWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget* pParent)
    : QWidget(pParent)
    , m_mainVM(std::move(mainVM))
{
    InitUI();
    BindVM();
}

ConstructsWidget::~ConstructsWidget()
{
    m_pConstructComboBox = nullptr;
    m_pEntitiesListWidget = nullptr;
}

void ConstructsWidget::InitUI()
{
    auto pLayout = new QVBoxLayout(this);

    //
    // Construct ComboBox
    //
    m_pConstructComboBox = new QComboBox();
    connect(m_pConstructComboBox, &QComboBox::currentIndexChanged, this, &ConstructsWidget::UI_OnConstructComboCurrentIndexChanged);
    UpdateConstructsComboBoxContents();

    //
    // Entities List
    //
    m_pEntitiesListWidget = new QListWidget();
    UpdateEntitiesListContents();

    pLayout->addWidget(m_pConstructComboBox);
    pLayout->addWidget(m_pEntitiesListWidget, 1);
}

void ConstructsWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnPackageChanged, this, &ConstructsWidget::VM_OnPackageChanged);
    connect(m_mainVM.get(), &MainWindowVM::VM_OnConstructChanged, this, &ConstructsWidget::VM_OnConstructChanged);
}

void ConstructsWidget::UI_OnConstructComboCurrentIndexChanged(int index)
{
    // TODO!

    if (index >= 0)
    {
        m_mainVM->OnConstructChanged(std::make_shared<Engine::Construct>(m_pConstructComboBox->itemText(index).toStdString()));
    }
    else
    {
        m_mainVM->OnConstructChanged(std::nullopt);
    }
}

void ConstructsWidget::VM_OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>&)
{
    UpdateConstructsComboBoxContents();
}

void ConstructsWidget::VM_OnConstructChanged(const std::optional<Engine::Construct::Ptr>&)
{
    UpdateEntitiesListContents();
}

void ConstructsWidget::UpdateConstructsComboBoxContents()
{
    //
    // Clear State
    //
    m_pConstructComboBox->clear();
    m_pConstructComboBox->setEnabled(false);

    //
    // Update State
    //
    const auto& package = m_mainVM->GetModel().package;
    if (!package) { return; }

    m_pConstructComboBox->setEnabled(true);

    for (const auto& constructName : (*package)->GetConstructResourceNames())
    {
        m_pConstructComboBox->addItem(QString::fromStdString(constructName));
    }
}

void ConstructsWidget::UpdateEntitiesListContents()
{
    //
    // Clear State
    //
    m_pEntitiesListWidget->clear();

    //
    // Update State
    //
    const auto selectedConstruct = m_mainVM->GetModel().construct;
    if (!selectedConstruct)
    {
        return;
    }

    m_pEntitiesListWidget->addItem(QString::fromStdString((*selectedConstruct)->GetName()));

}

}
