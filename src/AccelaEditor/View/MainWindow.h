/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_MAINWINDOW_H
#define ACCELAEDITOR_VIEW_MAINWINDOW_H

#include "../Thread/PackageLoadThread.h"
#include "../Util/WorkerThread.h"

#include "Accela/Platform/Package/PackageSource.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

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
    class QtFutureNotifier;
    class SceneSyncer;
    class PackageLoadThread;

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

            // Signals that manipulate the progress dialog
            void OnProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText);
            void OnProgressFinished();

            // Signals from PackageLoadThread
            void OnPackageLoadFinished(const std::expected<Engine::Package, unsigned int>& result);

            // Signals from the ViewModel
            void VM_OnPackageSelected(const std::optional<Engine::Package>& package);
            void VM_OnConstructSelected(const std::optional<Engine::Construct::Ptr>& construct);
            void VM_OnComponentInvalidated(const Engine::CEntity::Ptr& entity, const Engine::Component::Ptr& component);

        private:

            void InitUI();
            void InitWindow();
            void InitWidgets();
            void BindVM();

            void closeEvent(QCloseEvent* e) override;

            void DisplayProgressDialog(const QString& title, const unsigned int& minimumDurationMs);
            void LoadPackage(const std::filesystem::path& packageFilePath);
            void UpdateWindowTitle();

            template <typename ResultType>
            void RunThreadWithModalProgressDialog(const QString& progressTitle,
                                                  const QString& progressLabel,
                                                  const unsigned int& minimumDurationMs,
                                                  const std::function<ResultType(const WorkerThread::WorkControl&)>& runLogic,
                                                  const std::function<void(WorkerThread::ResultHolder::Ptr)>& resultLogic);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;

            QDockWidget* m_pResourcesDockWidget{nullptr};
            QDockWidget* m_pConstructsDockWidget{nullptr};
            QDockWidget* m_pEntitiesDockWidget{nullptr};
            QDockWidget* m_pEntityDockWidget{nullptr};

            AccelaWindow* m_pAccelaWindow{nullptr};
            std::unique_ptr<SceneSyncer> m_sceneEntitySyncer;

            // File actions
            QAction* m_pSavePackageAction{nullptr};
            QAction* m_pClosePackageAction{nullptr};

            // Window actions
            QAction* m_pResourcesWindowAction{nullptr};
            QAction* m_pConstructsWindowAction{nullptr};
            QAction* m_pEntitiesWindowAction{nullptr};
            QAction* m_pEntityWindowAction{nullptr};

            QProgressDialog* m_pProgressDialog{nullptr};

            std::unique_ptr<QtFutureNotifier> m_qtFutureNotifier;
            PackageLoadThread* m_pPackageLoadThread{nullptr};

            std::shared_ptr<MainWindowVM> m_vm;
    };
}

#endif //ACCELAEDITOR_VIEW_MAINWINDOW_H
