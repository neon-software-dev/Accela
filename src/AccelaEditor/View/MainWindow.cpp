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

#include "../Util/ErrorDialog.h"
#include "../EditorScene/EditorScene.h"

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
    m_pAccelaWindow = new AccelaWindow(m_logger, m_metrics, std::make_shared<EditorScene>());
    connect(m_pAccelaWindow, &AccelaWindow::OnSceneMessageReceived, this, &MainWindow::UI_OnSceneMessage);
    auto pAccelaWidget = QWidget::createWindowContainer(m_pAccelaWindow, this);
    setCentralWidget(pAccelaWidget);

    m_vm->AttachToAccelaWindow(m_pAccelaWindow);

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

    auto pEntitiesWidget = new EntitiesWidget(m_vm, m_pEntitiesDockWidget);
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
    connect(m_vm.get(), &MainWindowVM::VM_ErrorDialogShow, this, &MainWindow::VM_ErrorDialogShow);
    connect(m_vm.get(), &MainWindowVM::VM_ProgressDialogShow, this, &MainWindow::VM_ProgressDialogShow);
    connect(m_vm.get(), &MainWindowVM::VM_ProgressDialogUpdate, this, &MainWindow::VM_ProgressDialogUpdate);
    connect(m_vm.get(), &MainWindowVM::VM_ProgressDialogClose, this, &MainWindow::VM_ProgressDialogClose);
    connect(m_vm.get(), &MainWindowVM::VM_OnPackageChanged, this, &MainWindow::VM_OnPackageChanged);
    connect(m_vm.get(), &MainWindowVM::VM_OnSelectedEntitiesChanged, this, &MainWindow::VM_OnSelectedEntitiesChanged);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
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

    m_vm->OnLoadPackage(std::filesystem::path(packageFile.toStdString()));
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

    m_vm->OnLoadPackage(*packageFilePath);
}

void MainWindow::UI_OnMenuFile_SavePackageTriggered(bool)
{
    m_vm->OnSavePackage();
}

void MainWindow::UI_OnMenuFile_ClosePackageTriggered(bool)
{
    m_vm->OnClosePackage();
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

void MainWindow::VM_ErrorDialogShow(const std::string& title, const std::string& message)
{
    DisplayError(tr(title.c_str()), tr(message.c_str()));
}

void MainWindow::VM_ProgressDialogShow(const std::string&)
{
    /*assert(m_pProgressDialog == nullptr);
    if (m_pProgressDialog != nullptr)
    {
        return;
    }

    m_pProgressDialog = new QProgressDialog(this);
    m_pProgressDialog->setWindowTitle(tr(title.c_str()));
    m_pProgressDialog->setCancelButtonText(tr("Cancel"));
    m_pProgressDialog->setModal(true);
    m_pProgressDialog->setMinimumDuration((int)STANDARD_MIN_DURATION);
    m_pProgressDialog->setMaximum(1);
    m_pProgressDialog->setValue(0);

    connect(m_pProgressDialog, &QProgressDialog::canceled, m_vm.get(), &MainWindowVM::OnProgressCancelled);*/
}

void MainWindow::VM_ProgressDialogUpdate(unsigned int, unsigned int, const std::string&)
{
    /*assert(m_pProgressDialog != nullptr);

    if (m_pProgressDialog)
    {
        m_pProgressDialog->setValue((int)progress);
        m_pProgressDialog->setMaximum((int)total);
        m_pProgressDialog->setLabelText(tr(status.c_str()));
    }*/
}

void MainWindow::VM_ProgressDialogClose()
{
    /*assert(m_pProgressDialog != nullptr);

    if (m_pProgressDialog)
    {
        m_pProgressDialog->close();
        m_pProgressDialog = nullptr;
    }*/
}

void MainWindow::VM_OnPackageChanged(const std::optional<Engine::Package>& package)
{
    // Update the window title to contain the package name
    UpdateWindowTitle();

    // Update available actions for the package
    m_pSavePackageAction->setEnabled(package.has_value());
    m_pClosePackageAction->setEnabled(package.has_value());
}

void MainWindow::VM_OnSelectedEntitiesChanged(const std::unordered_set<Engine::EntityId>& eids)
{
    m_pAccelaWindow->EnqueueSceneMessage(std::make_shared<SetEntitiesHighlightedCommand>(eids));
}

void MainWindow::UI_OnSceneMessage(const Common::Message::Ptr& message)
{
    if (message->GetTypeIdentifier() == EntityClicked::TYPE) { OnEntityClickedSceneMessage(message); }
    else if (message->GetTypeIdentifier() == NothingClicked::TYPE) { OnNothingClickedSceneMessage(message); }
}

void MainWindow::OnEntityClickedSceneMessage(const Common::Message::Ptr& message)
{
    const auto entityClickedMessage = std::dynamic_pointer_cast<EntityClicked>(message);
    m_vm->OnEntityClicked(entityClickedMessage->eid, entityClickedMessage->requestingMultipleSelect);
}

void MainWindow::OnNothingClickedSceneMessage(const Common::Message::Ptr&)
{
    m_vm->OnNothingClicked();
}

}
