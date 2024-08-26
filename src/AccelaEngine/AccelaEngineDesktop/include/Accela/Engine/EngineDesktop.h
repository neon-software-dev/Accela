/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_ENGINEDESKTOP_H
#define LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_ENGINEDESKTOP_H

#include <Accela/Engine/Scene/Scene.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <memory>
#include <string>

namespace Accela::Platform
{
    class IPlatform;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    struct ACCELA_PUBLIC WindowParams
    {
        WindowParams(std::string _windowTitle,
                     const Render::USize& _windowSize)
             : windowTitle(std::move(_windowTitle))
             , windowSize(_windowSize)
        { }

        std::string windowTitle;
        Render::USize windowSize;
    };

    enum class VROutput
    {
        None,
        Optional,
        Required
    };

    /**
     * Helper class for Desktop-based clients to create a window and run the Accela engine
     */
    class ACCELA_PUBLIC EngineDesktop
    {
        public:

            EngineDesktop(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics);

            /**
             * Initialize/start the desktop system
             *
             * @return Whether or not the startup was successful
             */
            bool Startup();

            /**
             * Shut the desktop system down. (Cleans up post-run resources).
             */
            void Shutdown();

            /**
             * Run the Accela engine in a desktop window.
             *
             * @param appName The name of the client app.
             * @param appVersion The version of the client app.
             * @param windowParams Parameters defining the window to be created.
             * @param vrOutput Whether or not to attempt to output to a VR headset.
             * @param initialScene The initial scene to be run.
             */
            void Run(const std::string& appName,
                     const uint32_t& appVersion,
                     const WindowParams& windowParams,
                     VROutput vrOutput,
                     Scene::UPtr initialScene);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            std::shared_ptr<Platform::IPlatform> m_platform;
    };
}

#endif //LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_ENGINEDESKTOP_H
