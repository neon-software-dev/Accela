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

    //
    // Main layout
    //
    auto pLayout = new QVBoxLayout(this);
    pLayout->addWidget(m_pConstructsListWidget, 1);

    //
    // Initial contents update
    //
    UpdateConstructsListContents();
}

void ConstructsWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnPackageChanged, this, &ConstructsWidget::VM_OnPackageChanged);
    connect(m_mainVM.get(), &MainWindowVM::VM_OnConstructChanged, this, &ConstructsWidget::VM_OnConstructChanged);
}

void ConstructsWidget::UI_OnConstructsCurrentRowChanged(int index)
{
    // Ignore list selection events when we're updating the list's contents
    if (m_updatingConstructsList)
    {
        return;
    }

    // If no construct selected, tell the VM that
    if (index < 0)
    {
        m_mainVM->OnLoadConstruct(std::nullopt);
        return;
    }

    // Otherwise, tell the VM about the construct that's now selected
    const auto& constructs = m_mainVM->GetModel().package->constructs;

    if (index >= (int)constructs.size())
    {
        assert(false);
        return;
    }

    m_mainVM->OnLoadConstruct(constructs.at(index)->GetName());
}

void ConstructsWidget::VM_OnPackageChanged(const std::optional<Engine::Package>&)
{
    UpdateConstructsListContents();
}

void ConstructsWidget::VM_OnConstructChanged(const std::optional<Engine::Construct::Ptr>& construct)
{
    int index = -1;

    unsigned int currentIndex = 0;

    for (const auto& it : m_mainVM->GetModel().package->constructs)
    {
        if (it == construct)
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
