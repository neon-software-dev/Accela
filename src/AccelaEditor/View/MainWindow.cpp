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
#include "EntitiesWidget.h"
#include "EntityWidget.h"

#include "../EditorScene/SceneSyncer.h"

#include "../Util/ErrorDialog.h"
#include "../Util/QtFutureNotifier.h"

#include <Accela/Engine/Package/DiskPackage.h>

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

// Minimum time before progress dialog will display
static constexpr auto STANDARD_MIN_DURATION = 300; // ms

MainWindow::MainWindow(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics)
    : QMainWindow(nullptr)
    , m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_qtFutureNotifier(std::make_unique<QtFutureNotifier>(this))
    , m_vm(std::make_shared<MainWindowVM>(m_logger, MainWindowVM::Model{}))
{
    InitUI();
    BindVM();
}

MainWindow::~MainWindow()
{
    m_pResourcesDockWidget = nullptr;
    m_pConstructsDockWidget = nullptr;
    m_pEntitiesDockWidget = nullptr;
    m_pEntityDockWidget = nullptr;
    m_pAccelaWindow = nullptr;
    m_pSavePackageAction = nullptr;
    m_pClosePackageAction = nullptr;
    m_pResourcesWindowAction = nullptr;
    m_pProgressDialog = nullptr;
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
    setMinimumSize(800, 600);
    showMaximized();

    UpdateWindowTitle();

    //
    // Actions
    //

    // File actions
    auto pNewPackageAction = new QAction(tr("&New Package"), this);
    pNewPackageAction->setStatusTip(tr("Create a new Accela Package"));
    connect(pNewPackageAction, &QAction::triggered, this, &MainWindow::UI_OnMenuFile_NewPackageTriggered);

    auto pOpenPackageAction = new QAction(tr("&Open Package"), this);
    pOpenPackageAction->setStatusTip(tr("Open an Accela Package"));
    connect(pOpenPackageAction, &QAction::triggered, this, &MainWindow::UI_OnMenuFile_OpenPackageTriggered);

    m_pSavePackageAction = new QAction(tr("&Save Package"), this);
    m_pSavePackageAction->setStatusTip(tr("Save the current Package"));
    m_pSavePackageAction->setEnabled(false);
    connect(m_pSavePackageAction, &QAction::triggered, this, &MainWindow::UI_OnMenuFile_SavePackageTriggered);

    m_pClosePackageAction = new QAction(tr("&Close Package"), this);
    m_pClosePackageAction->setStatusTip(tr("Close the current Package"));
    m_pClosePackageAction->setEnabled(false);
    connect(m_pClosePackageAction, &QAction::triggered, this, &MainWindow::UI_OnMenuFile_ClosePackageTriggered);

    auto pExitAction = new QAction(tr("&Exit"), this);
    pExitAction->setStatusTip(tr("Exit Accela Editor"));
    connect(pExitAction, &QAction::triggered, this, &MainWindow::UI_OnMenuFile_ExitTriggered);

    const auto testIcon = QIcon("./assets/textures/blue.jpg");
    auto* pTestAct = new QAction(testIcon, tr("&Test..."), this);
    pTestAct->setStatusTip(tr("Test Action"));

    // Window actions
    m_pResourcesWindowAction = new QAction(tr("&Resources Window"), this);
    m_pResourcesWindowAction->setStatusTip(tr("Open the Resources Window"));
    m_pResourcesWindowAction->setEnabled(false);
    connect(m_pResourcesWindowAction, &QAction::triggered, this, &MainWindow::UI_OnMenuWindow_ResourcesTriggered);

    m_pConstructsWindowAction = new QAction(tr("&Constructs Window"), this);
    m_pConstructsWindowAction->setStatusTip(tr("Open the Constructs Window"));
    m_pConstructsWindowAction->setEnabled(false);
    connect(m_pConstructsWindowAction, &QAction::triggered, this, &MainWindow::UI_OnMenuWindow_ConstructsTriggered);

    m_pEntitiesWindowAction = new QAction(tr("&Entities Window"), this);
    m_pEntitiesWindowAction->setStatusTip(tr("Open the Entities Window"));
    m_pEntitiesWindowAction->setEnabled(false);
    connect(m_pEntitiesWindowAction, &QAction::triggered, this, &MainWindow::UI_OnMenuWindow_EntitiesTriggered);

    m_pEntityWindowAction = new QAction(tr("E&ntity Window"), this);
    m_pEntityWindowAction->setStatusTip(tr("Open the Entity Window"));
    m_pEntityWindowAction->setEnabled(false);
    connect(m_pEntityWindowAction, &QAction::triggered, this, &MainWindow::UI_OnMenuWindow_EntityTriggered);

    //
    // Menus
    //
    auto fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(pNewPackageAction);
    fileMenu->addAction(pOpenPackageAction);
    fileMenu->addAction(m_pSavePackageAction);
    fileMenu->addAction(m_pClosePackageAction);
    fileMenu->addAction(pExitAction);

    auto windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(m_pResourcesWindowAction);
    windowMenu->addAction(m_pConstructsWindowAction);
    windowMenu->addAction(m_pEntitiesWindowAction);
    windowMenu->addAction(m_pEntityWindowAction);

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

    m_sceneEntitySyncer = std::make_unique<SceneSyncer>(m_logger, m_pAccelaWindow);

    //
    // Package Resources Dock Widget
    //
    m_pResourcesDockWidget = new QDockWidget(tr("Package Resources"), this);
    m_pResourcesDockWidget->setMinimumSize(100,100);
    m_pResourcesDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    connect(m_pResourcesDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::UI_OnDockWidgetVisibilityChanged);
    addDockWidget(Qt::LeftDockWidgetArea, m_pResourcesDockWidget);

    auto pResourcesWidget = new ResourcesWidget(m_vm, m_pResourcesDockWidget);
    m_pResourcesDockWidget->setWidget(pResourcesWidget);

    //
    // Package Constructs Dock Widget
    //
    m_pConstructsDockWidget = new QDockWidget(tr("Package Constructs"), this);
    m_pConstructsDockWidget->setMinimumSize(100,100);
    m_pConstructsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    connect(m_pConstructsDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::UI_OnDockWidgetVisibilityChanged);
    addDockWidget(Qt::LeftDockWidgetArea, m_pConstructsDockWidget);

    auto pConstructsWidget = new ConstructsWidget(m_vm, m_pConstructsDockWidget);
    m_pConstructsDockWidget->setWidget(pConstructsWidget);

    //
    // Entities Dock Widget
    //
    m_pEntitiesDockWidget = new QDockWidget(tr("Construct Entities"), this);
    m_pEntitiesDockWidget->setMinimumSize(100,100);
    m_pEntitiesDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    connect(m_pEntitiesDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::UI_OnDockWidgetVisibilityChanged);
    addDockWidget(Qt::RightDockWidgetArea, m_pEntitiesDockWidget);

    auto pEntitiesWidget = new EntitiesWidget(m_vm, m_sceneEntitySyncer.get(), m_pEntitiesDockWidget);
    m_pEntitiesDockWidget->setWidget(pEntitiesWidget);

    //
    // Entity Dock Widget
    //
    m_pEntityDockWidget = new QDockWidget(tr("Entity"), this);
    m_pEntityDockWidget->setMinimumSize(100,100);
    m_pEntityDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    connect(m_pEntityDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::UI_OnDockWidgetVisibilityChanged);
    addDockWidget(Qt::RightDockWidgetArea, m_pEntityDockWidget);

    auto pEntityWidget = new EntityWidget(m_vm, m_pEntityDockWidget);
    m_pEntityDockWidget->setWidget(pEntityWidget);
}

void MainWindow::BindVM()
{
    connect(m_vm.get(), &MainWindowVM::VM_OnPackageSelected, this, &MainWindow::VM_OnPackageSelected);
    connect(m_vm.get(), &MainWindowVM::VM_OnConstructSelected, this, &MainWindow::VM_OnConstructSelected);
    connect(m_vm.get(), &MainWindowVM::VM_OnComponentInvalidated, this, &MainWindow::VM_OnComponentInvalidated);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    // Cancel and wait for the package load thread to finish, if one is running.
    //
    // Note: Qt will have automatically done work to forcefully stop the thread's
    // event loop by this point so, us running Cancel logic won't actually do anything.
    if (m_pPackageLoadThread != nullptr)
    {
        if (m_pPackageLoadThread->isRunning())
        {
            m_pPackageLoadThread->Cancel();
            m_pPackageLoadThread->wait(QDeadlineTimer::Forever);
        }

        m_pPackageLoadThread = nullptr;
    }

    m_qtFutureNotifier->Destroy();
    m_pAccelaWindow->Destroy();

    QWidget::closeEvent(e);
}

void MainWindow::UI_OnMenuFile_OpenPackageTriggered(bool)
{
    auto packageFile = QFileDialog::getOpenFileName(
        this,
        tr("Open Accela Package"),
        QString(),
        QString::fromStdString(std::format("Accela Packages (*{})", Platform::PACKAGE_EXTENSION))
    );
    if (packageFile.isEmpty())
    {
        return;
    }

    LoadPackage(std::filesystem::path(packageFile.toStdString()));
}

void MainWindow::UI_OnMenuFile_NewPackageTriggered(bool)
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

void MainWindow::UI_OnMenuFile_SavePackageTriggered(bool)
{
    const auto& package = (*m_vm->GetModel().package);
    const auto diskPackageSource = std::dynamic_pointer_cast<Platform::DiskPackageSource>(package.source);

    m_logger->Log(Common::LogLevel::Info, "MainWindow: Saving package: {}", package.manifest.GetPackageName());
    RunThreadWithModalProgressDialog<bool>(
        tr("Saving..."),
        tr("Writing package files"),
        STANDARD_MIN_DURATION,
        [=](const WorkerThread::WorkControl&) {
            return Engine::DiskPackage::WritePackageFilesToDisk(diskPackageSource->GetPackageDir(), package);
        }, [](const WorkerThread::ResultHolder::Ptr& result) {
            if (!WorkerThread::ResultAs<bool>(result))
            {
                DisplayError(tr("Failed to save the package"));
            }
        }
    );
}

void MainWindow::UI_OnMenuFile_ClosePackageTriggered(bool)
{
    const auto& package = (*m_vm->GetModel().package);

    m_logger->Log(Common::LogLevel::Info, "MainWindow: Closing package: {}", package.manifest.GetPackageName());
    RunThreadWithModalProgressDialog<bool>(
        tr("Closing..."),
        tr("Closing package"),
        STANDARD_MIN_DURATION,
        [=,this](const WorkerThread::WorkControl&) {
            m_sceneEntitySyncer->DestroyAllEntities().get();
            m_sceneEntitySyncer->DestroyAllResources().get();
            return true;
        }, [this](const WorkerThread::ResultHolder::Ptr& result) {
            if (!WorkerThread::ResultAs<bool>(result))
            {
                DisplayError(tr("Failed to close the package"));
            }

            m_vm->OnPackageSelected(std::nullopt);
        }
    );
}

void MainWindow::UI_OnMenuFile_ExitTriggered(bool)
{
    close();
}

void MainWindow::UI_OnMenuWindow_ResourcesTriggered(bool)
{
    m_pResourcesDockWidget->setVisible(true);
}

void MainWindow::UI_OnMenuWindow_ConstructsTriggered(bool)
{
    m_pConstructsDockWidget->setVisible(true);
}

void MainWindow::UI_OnMenuWindow_EntitiesTriggered(bool)
{
    m_pEntitiesDockWidget->setVisible(true);
}

void MainWindow::UI_OnMenuWindow_EntityTriggered(bool)
{
    m_pEntityDockWidget->setVisible(true);
}

void MainWindow::UI_OnDockWidgetVisibilityChanged(bool)
{
    // Update actions relevant to dock widgets
    m_pResourcesWindowAction->setEnabled(!m_pResourcesDockWidget->isVisible());
    m_pConstructsWindowAction->setEnabled(!m_pConstructsDockWidget->isVisible());
    m_pEntitiesWindowAction->setEnabled(!m_pEntitiesDockWidget->isVisible());
    m_pEntityWindowAction->setEnabled(!m_pEntityDockWidget->isVisible());
}

void MainWindow::OnProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText)
{
    if (m_pProgressDialog)
    {
        m_pProgressDialog->setValue((int)progress);
        m_pProgressDialog->setMaximum((int)total);
        m_pProgressDialog->setLabelText(QString::fromStdString(progressText));
    }
}

void MainWindow::OnProgressFinished()
{
    if (m_pProgressDialog)
    {
        m_pProgressDialog->close();
        m_pProgressDialog = nullptr;
    }
}

void MainWindow::OnPackageLoadFinished(const std::expected<Engine::Package, unsigned int>& result)
{
    m_pPackageLoadThread = nullptr;

    if (!result)
    {
        DisplayError(QString::fromStdString(std::format("Failed to open package, error code: {:#x}", result.error())));
        return;
    }

    m_vm->OnPackageSelected(*result);
}

void MainWindow::VM_OnPackageSelected(const std::optional<Engine::Package>& package)
{
    UpdateWindowTitle();

    // Update actions for the package
    m_pSavePackageAction->setEnabled(package.has_value());
    m_pClosePackageAction->setEnabled(package.has_value());
}

void MainWindow::VM_OnConstructSelected(const std::optional<Engine::Construct::Ptr>& construct)
{
    if (!construct)
    {
        return;
    }

    RunThreadWithModalProgressDialog<bool>(
        tr("Opening..."),
        tr("Opening construct"),
        STANDARD_MIN_DURATION,
        [=,this](const WorkerThread::WorkControl&) {
            m_sceneEntitySyncer->BlockingFullSyncConstruct(construct);
            return true;
        }, [](const WorkerThread::ResultHolder::Ptr& result) {
            if (!WorkerThread::ResultAs<bool>(result))
            {
                DisplayError(tr("Failed to open the construct"));
            }
        }
    );
}

void MainWindow::VM_OnComponentInvalidated(const Engine::CEntity::Ptr& entity, const Engine::Component::Ptr& component)
{
    (void)m_sceneEntitySyncer->UpdateEntityComponent(entity->name, component);
}

void MainWindow::UpdateWindowTitle()
{
    if (m_vm->GetModel().package)
    {
        setWindowTitle(QString::fromStdString(
            std::format("{} - {}", BASE_WINDOW_TITLE, m_vm->GetModel().package->manifest.GetPackageName()))
        );
    }
    else
    {
        setWindowTitle(QString::fromStdString(BASE_WINDOW_TITLE));
    }
}

void MainWindow::DisplayProgressDialog(const QString& title, const unsigned int& minimumDurationMs)
{
    assert(m_pProgressDialog == nullptr);

    m_pProgressDialog = new QProgressDialog(this);
    m_pProgressDialog->setWindowTitle(title);
    m_pProgressDialog->setCancelButtonText(tr("Cancel"));
    m_pProgressDialog->setModal(true);
    m_pProgressDialog->setMinimumDuration((int)minimumDurationMs);
}

void MainWindow::LoadPackage(const std::filesystem::path& packageFilePath)
{
    assert(m_pPackageLoadThread == nullptr);

    DisplayProgressDialog(tr("Loading package"), STANDARD_MIN_DURATION);

    // Create a PackageLoadThread instance which will load the package and send events to us about its progress
    //
    // Note: PackageLoadThread will destroy all entities/resources as needed to prepare for the new package
    m_pPackageLoadThread = new PackageLoadThread(this, m_sceneEntitySyncer.get(), packageFilePath);
    connect(m_pProgressDialog, &QProgressDialog::canceled, m_pPackageLoadThread, &PackageLoadThread::Cancel);
    connect(m_pPackageLoadThread, &PackageLoadThread::ProgressUpdate, this, &MainWindow::OnProgressUpdate);
    connect(m_pPackageLoadThread, &PackageLoadThread::PackageLoadFinished, this, &MainWindow::OnPackageLoadFinished);
    connect(m_pPackageLoadThread, &PackageLoadThread::finished, this, &MainWindow::OnProgressFinished);
    connect(m_pPackageLoadThread, &PackageLoadThread::finished, m_pPackageLoadThread, &QObject::deleteLater);

    m_pPackageLoadThread->start();
}

template<typename ResultType>
void MainWindow::RunThreadWithModalProgressDialog(const QString& progressTitle,
                                                  const QString& progressLabel,
                                                  const unsigned int& minimumDurationMs,
                                                  const std::function<ResultType(const WorkerThread::WorkControl&)>& runLogic,
                                                  const std::function<void(WorkerThread::ResultHolder::Ptr)>& resultLogic)
{
    DisplayProgressDialog(progressTitle, minimumDurationMs);
    m_pProgressDialog->setValue(0);
    m_pProgressDialog->setMaximum(1);
    m_pProgressDialog->setLabelText(progressLabel);

    auto workerThread = WorkerThread::Create<ResultType>(this, runLogic);

    // If the progress dialog is cancelled, pass that signal to the worker thread / worker logic
    connect(m_pProgressDialog, &QProgressDialog::canceled, workerThread, &WorkerThread::OnCancelled);

    // When the thread finishes running, close the progress dialog
    connect(workerThread, &WorkerThread::finished, this, &MainWindow::OnProgressFinished);

    // When the thread finishes running, run its result logic.
    //
    // Note: we do this on the 'finished' signal rather than the 'OnResult' signal so that the
    // progress dialog will have been closed before processing the result, so the result logic can
    // start a new progress dialog flow as desired without erroring out because a progress dialog is
    // still already open.
    connect(workerThread, &WorkerThread::finished, this, [=](){
        std::invoke(resultLogic, workerThread->GetResult());
    });

    // When the thread finishes running, schedule it to be deleted
    connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();
}

}
