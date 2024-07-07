/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_MAINWINDOW_H
#define ACCELAEDITOR_VIEW_MAINWINDOW_H

#include "../SceneMessageListener.h"

#include <Accela/Engine/Package/Package.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/Thread/Message.h>

#include <QMainWindow>

#include <memory>
#include <filesystem>

class QProgressDialog;
class QAction;
class QDockWidget;

namespace Accela
{
    class AccelaWindow;
    class MainWindowVM;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

        public:

            MainWindow(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics);
            ~MainWindow() override;

        private slots:

            // Signals from the UI
            void UI_OnMenuFile_OpenPackageTriggered(bool);
            void UI_OnMenuFile_NewPackageTriggered(bool);
            void UI_OnMenuFile_SavePackageTriggered(bool);
            void UI_OnMenuFile_ClosePackageTriggered(bool);
            void UI_OnMenuFile_ExitTriggered(bool);
            void UI_OnMenuWindow_ResourcesTriggered(bool);
            void UI_OnMenuWindow_ConstructsTriggered(bool);
            void UI_OnMenuWindow_EntitiesTriggered(bool);
            void UI_OnMenuWindow_EntityTriggered(bool);
            void UI_OnDockWidgetVisibilityChanged(bool);

            // Signals from AccelaWindow/Scene
            void UI_OnSceneMessage(const Common::Message::Ptr& message);

            // Signals from the ViewModel
            void VM_ErrorDialogShow(const std::string& title, const std::string& message);
            void VM_ProgressDialogShow(const std::string& title);
            void VM_ProgressDialogUpdate(unsigned int progress, unsigned int total, const std::string& status);
            void VM_ProgressDialogClose();
            void VM_OnPackageChanged(const std::optional<Engine::Package>& package);

        private:

            void InitUI();
            void InitWindow();
            void InitWidgets();
            void BindVM();

            void closeEvent(QCloseEvent* e) override;

            void UpdateWindowTitle();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;

            QDockWidget* m_pResourcesDockWidget{nullptr};
            QDockWidget* m_pConstructsDockWidget{nullptr};
            QDockWidget* m_pEntitiesDockWidget{nullptr};
            QDockWidget* m_pEntityDockWidget{nullptr};

            AccelaWindow* m_pAccelaWindow{nullptr};

            // File actions
            QAction* m_pSavePackageAction{nullptr};
            QAction* m_pClosePackageAction{nullptr};

            // Window actions
            QAction* m_pResourcesWindowAction{nullptr};
            QAction* m_pConstructsWindowAction{nullptr};
            QAction* m_pEntitiesWindowAction{nullptr};
            QAction* m_pEntityWindowAction{nullptr};

            QProgressDialog* m_pProgressDialog{nullptr};

            std::shared_ptr<MainWindowVM> m_vm;
    };
}

#endif //ACCELAEDITOR_VIEW_MAINWINDOW_H
