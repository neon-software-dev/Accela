/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "View/MainWindow.h"

#include <Accela/Common/Log/StdLogger.h>
#include <Accela/Common/Metrics/InMemoryMetrics.h>

#include <QApplication>

int main(int argc, char *argv[])
{
    using namespace Accela;

    QApplication a(argc, argv);

    const auto logger= std::make_shared<Common::StdLogger>(Common::LogLevel::Warning);
    const auto metrics= std::make_shared<Common::InMemoryMetrics>();

    auto pMainWindow = new MainWindow(logger, metrics);
    pMainWindow->show();

    QCoreApplication::exec();

    return 0;
}
