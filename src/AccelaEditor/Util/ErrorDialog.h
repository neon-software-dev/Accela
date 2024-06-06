/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_UTIL_ERRORDIALOG_H
#define ACCELAEDITOR_UTIL_ERRORDIALOG_H

#include <QMessageBox>

#include <string>

namespace Accela
{
    void DisplayError(const QString& message)
    {
        QMessageBox msgBox{};
        msgBox.setWindowTitle("Error");
        msgBox.setText(message);
        msgBox.exec();
    }
}

#endif //ACCELAEDITOR_UTIL_ERRORDIALOG_H
