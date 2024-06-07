/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MainWindowVM.h"

#include "../Util/ModelUpdate.h"
#include "../Thread/PackageLoadThread.h"
#include "../EditorScene/SceneSyncer.h"

#include <Accela/Engine/Package/DiskPackage.h>
#include <Accela/Engine/Package/CTransformComponent.h>
#include <Accela/Engine/Package/CModelRenderableComponent.h>

#include <format>
#include <ranges>
#include <algorithm>

namespace Accela
{

MainWindowVM::MainWindowVM(Common::ILogger::Ptr logger, MainWindowVM::Model model)
    : m_logger(std::move(logger))
    , m_sceneSyncer(std::make_unique<SceneSyncer>(m_logger))
    , m_model(std::move(model))
{

}

MainWindowVM::~MainWindowVM() = default;

void MainWindowVM::AttachToAccelaWindow(AccelaWindow *pAccelaWindow)
{
    m_sceneSyncer->AttachToAccelaWindow(pAccelaWindow);
}

void MainWindowVM::OnProgressCancelled()
{
    if (m_pPackageLoadThread != nullptr)
    {
        m_pPackageLoadThread->Cancel();
    }

    if (m_pWorkThread != nullptr)
    {
        m_pWorkThread->OnCancelled();
    }
}

void MainWindowVM::OnLoadPackage(const std::filesystem::path& packageFilePath)
{
    assert(m_sceneSyncer != nullptr);

    emit VM_ProgressDialogShow("Loading Package");

    m_pPackageLoadThread = new PackageLoadThread(this, m_sceneSyncer.get(), packageFilePath);
    connect(m_pPackageLoadThread, &PackageLoadThread::ProgressUpdate, this, &MainWindowVM::PLT_ProgressUpdate);
    connect(m_pPackageLoadThread, &PackageLoadThread::PackageLoadFinished, this, &MainWindowVM::PLT_PackageLoadFinished);
    connect(m_pPackageLoadThread, &PackageLoadThread::finished, this, &MainWindowVM::PLT_Finished);
    connect(m_pPackageLoadThread, &PackageLoadThread::finished, m_pPackageLoadThread, &QObject::deleteLater);

    m_pPackageLoadThread->start();
}

void MainWindowVM::OnSavePackage()
{
    assert(m_model.package.has_value());

    const auto& package = (*m_model.package);
    const auto diskPackageSource = std::dynamic_pointer_cast<Platform::DiskPackageSource>(package.source);

    RunThreadWithModelProgress<bool>(
        "Saving",
        "Saving Package",
        [=](const WorkerThread::WorkControl&){
            return Engine::DiskPackage::WritePackageFilesToDisk(diskPackageSource->GetPackageDir(), package);
        },
        [this](const WorkerThread::ResultHolder::Ptr& result){
            if (!WorkerThread::ResultAs<bool>(result))
            {
                emit VM_ErrorDialogShow("Error", "Failed to save the package");
            }
        });
}

void MainWindowVM::OnClosePackage()
{
    RunThreadWithModelProgress<bool>(
        "Closing",
        "Closing Package",
        [this](const WorkerThread::WorkControl&){
            m_sceneSyncer->DestroyAllEntities().get();
            m_sceneSyncer->DestroyAllResources().get();
            return true;
        },
        [this](const WorkerThread::ResultHolder::Ptr&){
            UpdateAndEmit(m_model.entity, {}, this, &MainWindowVM::VM_OnEntityChanged);
            UpdateAndEmit(m_model.construct, {}, this, &MainWindowVM::VM_OnConstructChanged);
            UpdateAndEmit(m_model.package, {}, this, &MainWindowVM::VM_OnPackageChanged);
        });
}

void MainWindowVM::OnLoadConstruct(const std::optional<std::string>& constructName)
{
    if (!constructName.has_value())
    {
        UpdateAndEmit(m_model.construct, {}, this, &MainWindowVM::VM_OnConstructChanged);
        return;
    }

    assert(m_model.package.has_value());

    const auto it = std::ranges::find_if(m_model.package->constructs, [&](const auto& construct){
        return construct->GetName() == constructName;
    });
    if (it == m_model.package->constructs.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "MainWindowVM::OnLoadConstruct: No such construct");
        return;
    }

    const auto construct = *it;

    RunThreadWithModelProgress<bool>(
        "Opening",
        "Opening Construct",
        [=,this](const WorkerThread::WorkControl&){
            m_sceneSyncer->BlockingFullSyncConstruct(construct);
            return true;
        },
        [=,this](const WorkerThread::ResultHolder::Ptr&){
            // Set the current construct
            if (UpdateAndEmit(m_model.construct, {construct}, this, &MainWindowVM::VM_OnConstructChanged))
            {
                m_logger->Log(Common::LogLevel::Info, "MainWindowVM::OnLoadConstruct: Construct changed: {}", construct->GetName());
            }

            // Unset the current entity now that we've changed constructs
            UpdateAndEmit(m_model.entity, {}, this, &MainWindowVM::VM_OnEntityChanged);
        });
}

void MainWindowVM::OnCreateEntity()
{
    assert(m_model.construct.has_value());

    const auto entity = std::make_shared<Engine::CEntity>(GetNewEntityName());

    // Add the entity to the model
    (*m_model.construct)->AddEntity(entity);

    // Add the entity to the scene
    m_sceneSyncer->BlockingCreateEntity(entity);

    // Notify that the construct data was invalidated
    emit VM_OnConstructInvalidated(*m_model.construct);
}

void MainWindowVM::OnLoadEntity(const std::optional<std::string>& entityName)
{
    if (!entityName.has_value())
    {
        UpdateAndEmit(m_model.entity, {}, this, &MainWindowVM::VM_OnEntityChanged);
        return;
    }

    assert(m_model.construct.has_value());

    const auto it = std::ranges::find_if((*m_model.construct)->GetEntities(), [&](const auto& entity){
        return entity->name == entityName;
    });
    if (it == (*m_model.construct)->GetEntities().cend())
    {
        m_logger->Log(Common::LogLevel::Error, "MainWindowVM::OnLoadEntity: No such entity");
        return;
    }

    std::optional<Engine::CEntity::Ptr> entity = (*it);
    if (UpdateAndEmit(m_model.entity, entity, this, &MainWindowVM::VM_OnEntityChanged))
    {
        const std::string entityStr = entity.has_value() ? (*entity)->name : "None";
        m_logger->Log(Common::LogLevel::Info, "MainWindowVM::OnLoadEntity: Entity changed: {}", entityStr);
    }
}

void MainWindowVM::OnDeleteEntity()
{
    assert(m_model.entity.has_value());

    // Remove the entity from the model
    (*m_model.construct)->RemoveEntity((*m_model.entity)->name);

    // Remove the entity from the scene
    (void)m_sceneSyncer->DestroyEntity((*m_model.entity)->name);

    // Unset the selected entity
    UpdateAndEmit(m_model.entity, {}, this, &MainWindowVM::VM_OnEntityChanged);

    // Notify that the construct data was invalidated
    emit VM_OnConstructInvalidated(*m_model.construct);
}

void MainWindowVM::OnCreateComponent(const Engine::Component::Type& type)
{
    assert(m_model.entity.has_value());

    Engine::Component::Ptr component;

    switch (type)
    {
        case Engine::Component::Type::Transform:
            component = std::make_shared<Engine::CTransformComponent>();
        break;
        case Engine::Component::Type::ModelRenderable:
            component = std::make_shared<Engine::CModelRenderableComponent>();
        break;
    }

    // Add the component to the model
    (*m_model.entity)->components.push_back(component);

    // Add the component to the scene
    (void)m_sceneSyncer->UpdateEntityComponent((*m_model.entity)->name, component);

    // Notify that the entity data is invalidated
    emit VM_OnEntityInvalidated(*m_model.entity);
}

void MainWindowVM::OnComponentModified(const Engine::Component::Ptr& component)
{
    assert(m_model.entity.has_value());

    // Sync the scene to the updated component
    (void)m_sceneSyncer->UpdateEntityComponent((*m_model.entity)->name, component);

    // Notify that the component data is invalidated
    emit VM_OnComponentInvalidated(*m_model.entity, component);
}

void MainWindowVM::PLT_ProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText)
{
    emit VM_ProgressDialogUpdate(progress, total, progressText);
}

void MainWindowVM::PLT_PackageLoadFinished(const std::expected<Engine::Package, unsigned int>& result)
{
    if (!result)
    {
        emit VM_ErrorDialogShow(
            "Package Load Error",
            std::format("Failed to load package, error code: {:#x}", result.error())
        );
        return;
    }

    std::optional<Engine::Package> package = *result;

    // Unset selected entity and construct now that we've loaded a new package
    UpdateAndEmit(m_model.entity, {}, this, &MainWindowVM::VM_OnEntityChanged);
    UpdateAndEmit(m_model.construct, {}, this, &MainWindowVM::VM_OnConstructChanged);

    if (UpdateAndEmit(m_model.package, package, this, &MainWindowVM::VM_OnPackageChanged))
    {
        const std::string packageStr = package.has_value() ? (*package).manifest.GetPackageName() : "None";
        m_logger->Log(Common::LogLevel::Info, "MainWindowVM::PLT_PackageLoadFinished: Package changed: {}", packageStr);
    }
}

void MainWindowVM::PLT_Finished()
{
    emit VM_ProgressDialogClose();
}

void MainWindowVM::WT_Finished()
{
    emit VM_ProgressDialogClose();
}

std::string MainWindowVM::GetNewEntityName() const
{
    unsigned int firstFreePostfix = 1;

    for (const auto& entity : (*m_model.construct)->GetEntities())
    {
        if (entity->name.starts_with(Engine::DEFAULT_CENTITY_NAME) &&
            entity->name.length() >= Engine::DEFAULT_CENTITY_NAME.length() + 2)
        {
            const auto postFix = entity->name.substr(Engine::DEFAULT_CENTITY_NAME.length());

            try
            {
                const auto postFixInt = std::stoi(postFix);

                firstFreePostfix = std::max((int)firstFreePostfix, postFixInt + 1);
            }
            catch (std::exception& e)
            { }
        }
    }

    return std::format("{} {}", Engine::DEFAULT_CENTITY_NAME, firstFreePostfix);
}

template<typename ResultType>
void MainWindowVM::RunThreadWithModelProgress(const std::string& title,
                                              const std::string& status,
                                              const std::function<ResultType(const WorkerThread::WorkControl&)>& runLogic,
                                              const std::function<void(const WorkerThread::ResultHolder::Ptr&)>& resultLogic)
{
    emit VM_ProgressDialogShow(title);
    emit VM_ProgressDialogUpdate(0, 1, status);

    auto workerThread = WorkerThread::Create<ResultType>(this, runLogic);
    connect(workerThread, &WorkerThread::finished, this, &MainWindowVM::WT_Finished);
    connect(workerThread, &WorkerThread::finished, this, [=](){
        std::invoke(resultLogic, workerThread->GetResult());
    });
    connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();
}

}
