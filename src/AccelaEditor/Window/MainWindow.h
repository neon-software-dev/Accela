/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_WINDOW_MAINWINDOW_H
#define ACCELAEDITOR_WINDOW_MAINWINDOW_H

#include <QMainWindow>

namespace Accela
{
    class AccelaWindow;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

        public:

            MainWindow();
            ~MainWindow() override;

        private:

            void InitUI();
            void InitWindow();
            void InitMenuBar();
            void InitWidgets();

            void closeEvent(QCloseEvent* e) override;

        private slots:

            void OnMenuExitTriggered(bool);

        private:

            AccelaWindow* m_pAccelaWindow{nullptr};
    };
}

#endif //ACCELAEDITOR_WINDOW_MAINWINDOW_H
