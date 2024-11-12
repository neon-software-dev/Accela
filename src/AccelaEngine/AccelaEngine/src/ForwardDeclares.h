/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_FORWARDDECLARES_H
#define LIBACCELAENGINE_SRC_FORWARDDECLARES_H

#include <memory>

namespace Accela::Engine
{
    class Scene; using ScenePtr = std::shared_ptr<Scene>;
    class IWorldResources; using IWorldResourcesPtr = std::shared_ptr<IWorldResources>;
    class IWorldState; using IWorldStatePtr = std::shared_ptr<IWorldState>;
    class AudioManager; using AudioManagerPtr = std::shared_ptr<AudioManager>;
    class MediaManager; using MediaManagerPtr = std::shared_ptr<MediaManager>;
    class IPhysics; using IPhysicsPtr = std::shared_ptr<IPhysics>;
    class IPhysicsRuntime; using IPhysicsRuntimePtr = std::shared_ptr<IPhysicsRuntime>;
    class IPackageResources; using IPackageResourcesPtr = std::shared_ptr<IPackageResources>;
    class PackageResources; using PackageResourcesPtr = std::shared_ptr<PackageResources>;
    class ITextureResources; using ITextureResourcesPtr = std::shared_ptr<ITextureResources>;
    class IFontResources; using IFontResourcesPtr = std::shared_ptr<IFontResources>;
    class IMaterialResources; using IMaterialResourcesPtr = std::shared_ptr<IMaterialResources>;
    class IMeshResources; using IMeshResourcesPtr = std::shared_ptr<IMeshResources>;
    class IModelResources; using IModelResourcesPtr = std::shared_ptr<IModelResources>;
}

#endif //LIBACCELAENGINE_SRC_FORWARDDECLARES_H
