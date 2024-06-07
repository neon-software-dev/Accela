/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H
#define ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H

#include "../Thread/WorkerThread.h"

#include <Accela/Engine/Package/Package.h>

#include <Accela/Common/Log/ILogger.h>

#include <QObject>

#include <optional>
#include <filesystem>

namespace Accela
{
    class PackageLoadThread;
    class SceneSyncer;
    class AccelaWindow;

    class MainWindowVM : public QObject
    {
        Q_OBJECT

        public:

            struct Model
            {
                std::optional<Engine::Package> package;
                std::optional<Engine::Construct::Ptr> construct;
                std::optional<Engine::CEntity::Ptr> entity;

                template <typename ComponentType>
                [[nodiscard]] std::optional<std::shared_ptr<ComponentType>>
                GetEntityComponent(const Engine::Component::Type& type) const;
            };

        public:

            MainWindowVM(Common::ILogger::Ptr logger, Model model);
            ~MainWindowVM() override;

            void AttachToAccelaWindow(AccelaWindow* pAccelaWindow);

            [[nodiscard]] const Model& GetModel() const noexcept { return m_model; }

            // UI event handlers
            void OnProgressCancelled();

            void OnLoadPackage(const std::filesystem::path& packageFilePath);
            void OnSavePackage();
            void OnClosePackage();

            void OnLoadConstruct(const std::optional<std::string>& constructName);

            void OnCreateEntity();
            void OnLoadEntity(const std::optional<std::string>& entityName);
            void OnDeleteEntity();

            void OnCreateComponent(const Engine::Component::Type& type);
            void OnComponentModified(const Engine::Component::Ptr& component);

        signals:

            void VM_ErrorDialogShow(const std::string& title, const std::string& message);

            void VM_ProgressDialogShow(const std::string& title);
            void VM_ProgressDialogUpdate(unsigned int progress, unsigned int total, const std::string& status);
            void VM_ProgressDialogClose();

            void VM_OnPackageChanged(const std::optional<Engine::Package>& package);

            void VM_OnConstructChanged(const std::optional<Engine::Construct::Ptr>& construct);
            void VM_OnConstructInvalidated(const Engine::Construct::Ptr& construct);

            void VM_OnEntityChanged(const std::optional<Engine::CEntity::Ptr>& entity);
            void VM_OnEntityInvalidated(const Engine::CEntity::Ptr& entity);

            void VM_OnComponentInvalidated(const Engine::CEntity::Ptr& entity, const Engine::Component::Ptr& component);

        private slots:

            // Signals from PackageLoadThread
            void PLT_ProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText);
            void PLT_PackageLoadFinished(const std::expected<Engine::Package, unsigned int>& result);
            void PLT_Finished();

            // Signals from WorkerThread
            void WT_Finished();

        private:

            [[nodiscard]] std::string GetNewEntityName() const;

            template<typename ResultType>
            void RunThreadWithModelProgress(const std::string& title,
                                            const std::string& status,
                                            const std::function<ResultType(const WorkerThread::WorkControl&)>& runLogic,
                                            const std::function<void(const WorkerThread::ResultHolder::Ptr&)>& resultLogic);

        private:

            Common::ILogger::Ptr m_logger;
            std::unique_ptr<SceneSyncer> m_sceneSyncer;
            Model m_model;

            PackageLoadThread* m_pPackageLoadThread{nullptr};
            WorkerThread* m_pWorkThread{nullptr};
    };

    template <typename ComponentType>
    [[nodiscard]] std::optional<std::shared_ptr<ComponentType>>
    MainWindowVM::Model::GetEntityComponent(const Engine::Component::Type& type) const
    {
        if (!entity) { return std::nullopt; }
        const auto component = (*entity)->GetComponent(type);
        if (!component) { return std::nullopt; }
        return std::dynamic_pointer_cast<ComponentType>(*component);
    }
}

#endif //ACCELAEDITOR_VIEWMODEL_MAINWINDOWVM_H
