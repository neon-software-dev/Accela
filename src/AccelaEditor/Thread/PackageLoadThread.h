/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_THREAD_PACKAGELOADTHREAD_H
#define ACCELAEDITOR_THREAD_PACKAGELOADTHREAD_H

#include "../View/AccelaWindow.h"

#include "Accela/Platform/Package/PackageSource.h"

#include <QThread>

#include <memory>
#include <filesystem>
#include <expected>
#include <string>

namespace Accela
{
    namespace Engine
    {
        class IPackageResources;
    }

    class PackageLoadThread : public QThread
    {
        Q_OBJECT

        public:

            PackageLoadThread(QObject* pParent, AccelaWindow* pAccelaWindow, std::filesystem::path packageFilePath);

        signals:

            void ProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText);
            void PackageLoadFinished(const std::expected<Platform::PackageSource::Ptr, unsigned int>& result);

        protected:

            void run() override;

        private:

            AccelaWindow* m_pAccelaWindow;

            std::filesystem::path m_packageFilePath;
    };
}

#endif //ACCELAEDITOR_THREAD_PACKAGELOADTHREAD_H
