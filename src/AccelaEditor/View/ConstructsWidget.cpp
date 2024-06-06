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
    m_pConstructsListWidget = nullptr;
}

void ConstructsWidget::InitUI()
{
    //
    // Constructs List
    //
    m_pConstructsListWidget = new QListWidget();
    connect(m_pConstructsListWidget, &QListWidget::currentRowChanged, this, &ConstructsWidget::UI_OnConstructsCurrentRowChanged);
    UpdateConstructsListContents();

    //
    // Main layout
    //
    auto pLayout = new QVBoxLayout(this);
    pLayout->addWidget(m_pConstructsListWidget, 1);
}

void ConstructsWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnPackageSelected, this, &ConstructsWidget::VM_OnPackageSelected);
    connect(m_mainVM.get(), &MainWindowVM::VM_OnConstructSelected, this, &ConstructsWidget::VM_OnConstructSelected);
}

void ConstructsWidget::UI_OnConstructsCurrentRowChanged(int index)
{
    // Ignore list selection events when we're updating the list's contents
    if (m_updatingConstructsList)
    {
        return;
    }

    if (index < 0)
    {
        m_mainVM->OnConstructSelected(std::nullopt);
        return;
    }

    const auto& constructs = m_mainVM->GetModel().package->constructs;

    if (index >= (int)constructs.size())
    {
        assert(false);
        return;
    }

    m_mainVM->OnConstructSelected(constructs.at(index));
}

void ConstructsWidget::VM_OnPackageSelected(const std::optional<Engine::Package>&)
{
    UpdateConstructsListContents();
}

void ConstructsWidget::VM_OnConstructSelected(const std::optional<Engine::Construct::Ptr>& selected)
{
    int index = -1;

    unsigned int currentIndex = 0;

    for (const auto& construct : m_mainVM->GetModel().package->constructs)
    {
        if (construct == selected)
        {
            index = (int)currentIndex;
        }

        currentIndex++;
    }

    m_pConstructsListWidget->setCurrentRow(index);
}

void ConstructsWidget::UpdateConstructsListContents()
{
    m_updatingConstructsList = true;

    //
    // Clear State
    //
    m_pConstructsListWidget->clear();

    //
    // Update State
    //
    const auto& package = m_mainVM->GetModel().package;
    if (!package)
    {
        m_updatingConstructsList = false;
        return;
    }

    int selectedRow = -1;
    unsigned int currentRow = 0;

    for (const auto& construct : package->constructs)
    {
        if (construct == m_mainVM->GetModel().construct)
        {
            selectedRow = (int)currentRow;
        }

        m_pConstructsListWidget->addItem(QString::fromStdString(construct->GetName()));
        currentRow++;
    }

    m_pConstructsListWidget->setCurrentRow(selectedRow);

    m_updatingConstructsList = false;
}

}
