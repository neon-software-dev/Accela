/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MainWindow.h"
#include "AccelaWindow.h"
#include "CreatePackageDialog.h"
#include "ResourcesWidget.h"
#include "ConstructsWidget.h"

#include "../Thread/PackageLoadThread.h"

#include "../ViewModel/MainWindowVM.h"

#include "../Util/ErrorDialog.h"
#include "../Util/QtFutureNotifier.h"

#include <Accela/Platform/File/IFiles.h>

#include <QMenuBar>
#include <QDockWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QProgressDialog>

#include <format>
#include <cassert>

namespace Accela
{

static constexpr auto BASE_WINDOW_TITLE = "Accela Editor";

MainWindow::MainWindow(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics)
    : QMainWindow(nullptr)
    , m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_qtFutureNotifier(std::make_unique<QtFutureNotifier>(this))
    , m_vm(std::make_shared<MainWindowVM>(MainWindowVM::Model{}))
{
    InitUI();
    BindVM();
}

MainWindow::~MainWindow()
{
    m_qtFutureNotifier->Destroy();
    m_qtFutureNotifier = nullptr;

    m_pPackageLoadProgressDialog = nullptr;
    m_pAccelaWindow = nullptr;
}

void MainWindow::InitUI()
{
    InitWindow();
    InitWidgets();
}

void MainWindow::InitWindow()
{
    //
    // Window General
    //
    UpdateWindowTitle();
    resize(1000, 1000); // Initial window size

    //
    // Actions
    //
    auto pNewPackageAction = new QAction(tr("&New Package"), this);
    pNewPackageAction->setStatusTip(tr("Create a new Accela Package"));
    connect(pNewPackageAction, &QAction::triggered, this, &MainWindow::OnMenuNewPackageTriggered);

    auto pOpenPackageAction = new QAction(tr("&Open Package"), this);
    pOpenPackageAction->setStatusTip(tr("Open an Accela Package"));
    connect(pOpenPackageAction, &QAction::triggered, this, &MainWindow::OnMenuOpenPackageTriggered);

    m_pClosePackageAction = new QAction(tr("&Close Package"), this);
    m_pClosePackageAction->setStatusTip(tr("Close the current Package"));
    m_pClosePackageAction->setEnabled(false);
    connect(m_pClosePackageAction, &QAction::triggered, this, &MainWindow::OnMenuClosePackageTriggered);

    auto pExitAction = new QAction(tr("&Exit"), this);
    pExitAction->setStatusTip(tr("Exit Accela Editor"));
    connect(pExitAction, &QAction::triggered, this, &MainWindow::OnMenuExitTriggered);

    const auto testIcon = QIcon("./assets/textures/blue.jpg");
    auto* pTestAct = new QAction(testIcon, tr("&Test..."), this);
    pTestAct->setStatusTip(tr("Test Action"));

    //
    // Menus
    //
    auto fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(pNewPackageAction);
    fileMenu->addAction(pOpenPackageAction);
    fileMenu->addAction(m_pClosePackageAction);
    fileMenu->addAction(pExitAction);

    //
    // Toolbars
    //
    auto* pTestToolBar = addToolBar(tr("File"));
    pTestToolBar->addAction(pTestAct);

    //
    // Status Bar
    //
    setStatusBar(new QStatusBar());
}

void MainWindow::InitWidgets()
{
    //
    // Central Accela Widget
    //
    m_pAccelaWindow = new AccelaWindow(m_logger, m_metrics);
    auto pAccelaWidget = QWidget::createWindowContainer(m_pAccelaWindow, this);
    setCentralWidget(pAccelaWidget);

    //
    // Package Resources Dock Widget
    //
    auto *pAssetsDockWidget = new QDockWidget("Package Resources", this);
    pAssetsDockWidget->setMinimumSize(100,100);
    pAssetsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, pAssetsDockWidget);

    auto pResourcesWidget = new ResourcesWidget(m_vm, pAssetsDockWidget);
    pAssetsDockWidget->setWidget(pResourcesWidget);

    //
    // Package Constructs Dock Widget
    //
    auto *pConstructsDockWidget = new QDockWidget("Package Constructs", this);
    pConstructsDockWidget->setMinimumSize(100,100);
    pConstructsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, pConstructsDockWidget);

    auto pConstructsWidget = new ConstructsWidget(m_vm, pConstructsDockWidget);
    pConstructsDockWidget->setWidget(pConstructsWidget);
}

void MainWindow::BindVM()
{
    connect(m_vm.get(), &MainWindowVM::VM_OnPackageChanged, this, &MainWindow::VM_OnPackageChanged);
}

void MainWindow::OnMenuOpenPackageTriggered(bool)
{
    auto packageFile = QFileDialog::getOpenFileName(
        this,
        tr("Open Accela PackageSource"),
        QString(),
        QString::fromStdString(std::format("Accela Packages (*{})", Platform::PACKAGE_EXTENSION))
    );
    if (packageFile.isEmpty())
    {
        return;
    }

    LoadPackage(std::filesystem::path(packageFile.toStdString()));
}

void MainWindow::OnMenuNewPackageTriggered(bool)
{
    auto pCreateDialog = new CreatePackageDialog(this);
    pCreateDialog->exec();

    const auto packageFilePath = pCreateDialog->GetResult();
    if (!packageFilePath)
    {
        return;
    }

    LoadPackage(*packageFilePath);
}

void MainWindow::OnMenuClosePackageTriggered(bool)
{
    const auto msg = std::make_shared<DestroySceneResourcesCommand>();
    m_qtFutureNotifier->EmitWhenFinished(msg->CreateFuture(), this, &MainWindow::OnPackageCloseFinished);
    m_pAccelaWindow->EnqueueSceneMessage(msg);
}

void MainWindow::OnPackageCloseFinished(const bool&)
{
    m_vm->OnPackageChanged(std::nullopt);
}

void MainWindow::OnMenuExitTriggered(bool)
{
    close();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    m_pAccelaWindow->Destroy();
    m_pAccelaWindow = nullptr;

    QWidget::closeEvent(e);
}

void MainWindow::OnPackageLoadProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText)
{
    if (m_pPackageLoadProgressDialog)
    {
        m_pPackageLoadProgressDialog->setValue((int)progress);
        m_pPackageLoadProgressDialog->setMaximum((int)total);
        m_pPackageLoadProgressDialog->setLabelText(QString::fromStdString(progressText));
    }
};

void MainWindow::OnPackageLoadFinished(const std::expected<Platform::PackageSource::Ptr, unsigned int>& result)
{
    if (m_pPackageLoadProgressDialog)
    {
        m_pPackageLoadProgressDialog->close();
        m_pPackageLoadProgressDialog = nullptr;
    }

    if (!result)
    {
        DisplayError(std::format("Failed to open package, error code: {:#x}", result.error()));
        return;
    }

    m_vm->OnPackageChanged(*result);
}

void MainWindow::VM_OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>& package)
{
    UpdateWindowTitle();

    // Update actions for the package
    m_pClosePackageAction->setEnabled(package.has_value());
}

void MainWindow::UpdateWindowTitle()
{
    if (m_vm->GetModel().package)
    {
        setWindowTitle(QString::fromStdString(std::format("{} - {}", BASE_WINDOW_TITLE, (*m_vm->GetModel().package)->GetPackageName())));
    }
    else
    {
        setWindowTitle(QString::fromStdString(BASE_WINDOW_TITLE));
    }
}

void MainWindow::LoadPackage(const std::filesystem::path& packageFilePath)
{
    assert(m_pPackageLoadProgressDialog == nullptr);

    m_pPackageLoadProgressDialog =  new QProgressDialog(this);
    m_pPackageLoadProgressDialog->setWindowTitle(tr("Opening package"));
    m_pPackageLoadProgressDialog->setCancelButtonText(tr("Cancel"));
    m_pPackageLoadProgressDialog->setModal(true);
    m_pPackageLoadProgressDialog->setMinimumDuration(500);

    // Unload any resources from a previously loaded package
    m_pAccelaWindow->EnqueueSceneMessage(std::make_shared<DestroySceneResourcesCommand>());

    // Create a PackageLoadThread instance which will load the package and send events to us about its progress
    auto pPackageLoadThread = new PackageLoadThread(this, m_pAccelaWindow, packageFilePath);
    connect(pPackageLoadThread, &PackageLoadThread::ProgressUpdate, this, &MainWindow::OnPackageLoadProgressUpdate);
    connect(pPackageLoadThread, &PackageLoadThread::PackageLoadFinished, this, &MainWindow::OnPackageLoadFinished);
    connect(pPackageLoadThread, &PackageLoadThread::finished, pPackageLoadThread, &QObject::deleteLater);
    pPackageLoadThread->start();
}

}
