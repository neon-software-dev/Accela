/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Window/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    using namespace Accela;

    QApplication a(argc, argv);

    auto* pMainWindow = new MainWindow();
    pMainWindow->show();

    QCoreApplication::exec();

    return 0;
}
