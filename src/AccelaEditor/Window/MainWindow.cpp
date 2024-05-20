/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MainWindow.h"
#include "AccelaWindow.h"

#include <QMenuBar>
#include <QDockWidget>

namespace Accela
{

MainWindow::MainWindow()
    : QMainWindow(nullptr)
{
    InitUI();
}

// For VM forward declare
MainWindow::~MainWindow() = default;

void MainWindow::InitUI()
{
    InitWindow();
    InitMenuBar();
    InitWidgets();
}

void MainWindow::InitWindow()
{
    setWindowTitle("Accela Editor");
    resize(1000, 1000); // Initial window size
}

void MainWindow::InitMenuBar()
{
    //
    // Actions
    //
    auto pExitAction = new QAction(tr("&Exit"), this);
    pExitAction->setStatusTip(tr("Exit Accela Editor"));
    connect(pExitAction, &QAction::triggered, this, &MainWindow::OnMenuExitTriggered);

    //
    // Menus
    //
    auto fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(pExitAction);
}

void MainWindow::InitWidgets()
{
    //
    // Central Accela Widget
    //
    m_pAccelaWindow = new AccelaWindow();
    auto pAccelaWidget = QWidget::createWindowContainer(m_pAccelaWindow, this);
    setCentralWidget(pAccelaWidget);

    //
    // Test Left Dock Widget
    //
    auto *pDockWidget = new QDockWidget("Test Panel", this);
    pDockWidget->setMinimumSize(300,100);
    pDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, pDockWidget);
}

void MainWindow::OnMenuExitTriggered(bool)
{
    close();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    m_pAccelaWindow->Destroy();

    QWidget::closeEvent(e);
}

}
