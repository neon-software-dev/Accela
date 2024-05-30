/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ResourcesWidget.h"

#include "../ViewModel/MainWindowVM.h"

#include <QListWidget>
#include <QBoxLayout>
#include <QComboBox>

#include <vector>
#include <string>

namespace Accela
{

ResourcesWidget::ResourcesWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget *pParent)
    : QWidget(pParent)
    , m_mainVM(std::move(mainVM))
{
    InitUI();
    BindVM();
}

ResourcesWidget::~ResourcesWidget()
{
    m_pTypeComboBox = nullptr;
    m_pResourcesListWidget = nullptr;
}

void ResourcesWidget::InitUI()
{
    auto pLayout = new QVBoxLayout(this);

    //
    // Type ComboBox
    //
    m_pTypeComboBox = new QComboBox();
    m_pTypeComboBox->addItem(QString(tr("Audio")));
    m_pTypeComboBox->addItem(QString(tr("Fonts")));
    m_pTypeComboBox->addItem(QString(tr("Textures")));
    m_pTypeComboBox->addItem(QString(tr("Models")));

    connect(m_pTypeComboBox, &QComboBox::currentIndexChanged, this, &ResourcesWidget::UI_TypeComboCurrentIndexChanged);

    //
    // Resources List Widget
    //
    m_pResourcesListWidget = new QListWidget();
    UpdateResourcesListContents();

    pLayout->addWidget(m_pTypeComboBox);
    pLayout->addWidget(m_pResourcesListWidget, 1);
}

void ResourcesWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnPackageChanged, this, &ResourcesWidget::VM_OnPackageChanged);
}

void ResourcesWidget::UI_TypeComboCurrentIndexChanged(int)
{
    UpdateResourcesListContents();
}

void ResourcesWidget::VM_OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>&)
{
    UpdateResourcesListContents();
}

void ResourcesWidget::UpdateResourcesListContents()
{
    //
    // Clear State
    //
    m_pResourcesListWidget->clear();
    m_pTypeComboBox->setEnabled(false);

    //
    // Update State
    //
    const auto& package = m_mainVM->GetModel().package;
    if (!package)
    {
        return;
    }

    m_pTypeComboBox->setEnabled(true);

    const auto currentIndex = m_pTypeComboBox->currentIndex();
    if (currentIndex < 0)
    {
        return;
    }

    std::vector<std::string> resourceNames;

    switch (currentIndex)
    {
        case 0: { resourceNames = (*package)->GetAudioResourceNames(); } break;
        case 1: { resourceNames = (*package)->GetFontResourceNames(); } break;
        case 2: { resourceNames = (*package)->GetTextureResourceNames(); } break;
        case 3: { resourceNames = (*package)->GetModelResourceNames(); } break;
        default: { /* no-op */ }
    }

    for (const auto& resourceName: resourceNames)
    {
        m_pResourcesListWidget->addItem(new QListWidgetItem(QString::fromStdString(resourceName), m_pResourcesListWidget));
    }
}

}
