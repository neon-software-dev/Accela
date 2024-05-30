/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_MAINWINDOW_H
#define ACCELAEDITOR_VIEW_MAINWINDOW_H

#include "Accela/Platform/Package/PackageSource.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <QMainWindow>

#include <memory>
#include <filesystem>

class QProgressDialog;
class QAction;

namespace Accela
{
    class AccelaWindow;
    class MainWindowVM;
    class QtFutureNotifier;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

        public:

            MainWindow(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics);
            ~MainWindow() override;

        private slots:

            void OnMenuOpenPackageTriggered(bool);
            void OnMenuNewPackageTriggered(bool);
            void OnMenuClosePackageTriggered(bool);
            void OnMenuExitTriggered(bool);

            void OnPackageLoadProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText);
            void OnPackageLoadFinished(const std::expected<Platform::PackageSource::Ptr, unsigned int>& result);
            void OnPackageCloseFinished(const bool&);

            void VM_OnPackageChanged(const std::optional<Platform::PackageSource::Ptr>& package);

        private:

            void InitUI();
            void InitWindow();
            void InitWidgets();
            void BindVM();

            void LoadPackage(const std::filesystem::path& packageFilePath);

            void closeEvent(QCloseEvent* e) override;

            void UpdateWindowTitle();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;

            AccelaWindow* m_pAccelaWindow{nullptr};
            QAction* m_pClosePackageAction{nullptr};

            QProgressDialog* m_pPackageLoadProgressDialog{nullptr};

            std::unique_ptr<QtFutureNotifier> m_qtFutureNotifier;

            std::shared_ptr<MainWindowVM> m_vm;
    };
}

#endif //ACCELAEDITOR_VIEW_MAINWINDOW_H
