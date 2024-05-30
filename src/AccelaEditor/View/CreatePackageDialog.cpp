/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "CreatePackageDialog.h"

#include "../ViewModel/PackageSelectVM.h"

#include <Accela/Engine/Package/DiskPackage.h>

#include <QPushButton>
#include <QBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QFormLayout>

#include <format>

namespace Accela
{

CreatePackageDialog::CreatePackageDialog(QWidget* pParent)
    : QDialog(pParent)
    , m_vm(std::make_unique<PackageSelectVM>())
{
    InitUI();
    BindVM();
}

void CreatePackageDialog::InitUI()
{
    //
    // Window General
    //
    setWindowTitle("Create Accela Package");

    //
    // Form Panel
    //

    // Name Row
    m_pCreateNameEdit = new QLineEdit();
    connect(m_pCreateNameEdit, &QLineEdit::textChanged, this, &CreatePackageDialog::UI_OnCreateNameChanged);

    // Directory Row
    m_pCreateDirectoryEdit = new QLineEdit();
    m_pCreateDirectoryEdit->setReadOnly(true);
    m_pCreateDirectoryEdit->setMinimumWidth(300);
    connect(m_pCreateDirectoryEdit, &QLineEdit::textChanged, this,
            &CreatePackageDialog::UI_OnCreateDirectoryChanged);

    auto pCreateDirectoryButton = new QPushButton(tr("..."));
    connect(pCreateDirectoryButton, &QPushButton::clicked, this,
            &CreatePackageDialog::OnCreateDirectoryButtonClicked);

    auto pDirectoryLayout = new QHBoxLayout();
    pDirectoryLayout->addWidget(m_pCreateDirectoryEdit, 1);
    pDirectoryLayout->addWidget(pCreateDirectoryButton);

    // Form layout
    auto pFormLayout = new QFormLayout();
    pFormLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pFormLayout->setLabelAlignment(Qt::AlignLeft);
    pFormLayout->addRow(tr("Name"),m_pCreateNameEdit);
    pFormLayout->addRow(tr("Location"), pDirectoryLayout);

    //
    // Create Button
    //
    m_pCreateButton = new QPushButton(tr("Create"));
    m_pCreateButton->setEnabled(false);
    connect(m_pCreateButton, &QPushButton::clicked, this, &CreatePackageDialog::OnCreateButtonClicked);

    //
    // Main layout
    //
    auto pPanelLayout = new QVBoxLayout(this);
    pPanelLayout->addLayout(pFormLayout);
    pPanelLayout->addWidget(m_pCreateButton);
}

void CreatePackageDialog::BindVM()
{
    connect(m_vm.get(), &PackageSelectVM::VM_OnCreateNameChanged, this, &CreatePackageDialog::VM_OnCreateNameChanged);
    connect(m_vm.get(), &PackageSelectVM::VM_OnCreateDirectoryChanged, this, &CreatePackageDialog::VM_OnCreateDirectoryChanged);
    connect(m_vm.get(), &PackageSelectVM::VM_OnCreateActionValidChanged, this, &CreatePackageDialog::VM_OnCreateActionValidChanged);
}

void CreatePackageDialog::OnCreateDirectoryButtonClicked(bool)
{
    auto directory = QFileDialog::getExistingDirectory(this, tr("Create Accela Package"));
    if (directory.isEmpty())
    {
        return;
    }

    m_pCreateDirectoryEdit->setText(directory);
}

void CreatePackageDialog::OnCreateButtonClicked(bool)
{
    //
    // Create an empty package
    //
    const auto createDirectory = m_vm->GetModel().createDirectory.value();
    const auto createName = m_vm->GetModel().createName.value();

    const auto createResult = Engine::DiskPackage::CreateOnDisk(createDirectory, Engine::PackageName(createName));
    if (!createResult)
    {
        DisplayErrorMessage(
            std::format("Failed to create package, error code: {}", (unsigned int)createResult.error())
        );
    }
    else
    {
        m_result = *createResult;
        close();
    }
}

void CreatePackageDialog::UI_OnCreateNameChanged(const QString& text)
{
    m_vm->OnCreateNameChanged(text.toStdString());
}

void CreatePackageDialog::UI_OnCreateDirectoryChanged(const QString& text)
{
    m_vm->OnCreateDirectoryChanged(text.toStdString());
}

void CreatePackageDialog::VM_OnCreateNameChanged(const std::optional<std::string>& createName)
{
    if (createName)
    {
        m_pCreateNameEdit->setText(QString::fromStdString(*createName));
    }
    else
    {
        m_pCreateNameEdit->setText("");
    }
}

void CreatePackageDialog::VM_OnCreateDirectoryChanged(const std::optional<std::filesystem::path>& createDirectory)
{
    if (createDirectory)
    {
        m_pCreateDirectoryEdit->setText(createDirectory->c_str());
    }
    else
    {
        m_pCreateDirectoryEdit->setText("");
    }
}

void CreatePackageDialog::VM_OnCreateActionValidChanged(const bool& createActionValid)
{
    m_pCreateButton->setEnabled(createActionValid);
}

void CreatePackageDialog::DisplayErrorMessage(const std::string& errorMsg)
{
    QMessageBox msgBox{};
    msgBox.setText(QString::fromStdString(errorMsg));
    msgBox.exec();
}

}
