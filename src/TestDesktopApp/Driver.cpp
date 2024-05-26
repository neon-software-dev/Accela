/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/EngineDesktop.h>
#include <Accela/Common/Log/StdLogger.h>
#include <Accela/Common/Metrics/InMemoryMetrics.h>

#include "TestScene.h"
#include "DevScene.h"

int main()
{
    using namespace Accela;

    auto logger = std::make_shared<Common::StdLogger>(Common::LogLevel::Warning);
    auto metrics = std::make_shared<Common::InMemoryMetrics>();

    auto desktopEngine = Engine::EngineDesktop(logger, metrics);

    if (!desktopEngine.Startup())
    {
        return 1;
    }

    desktopEngine.Run(
        "TestDesktopApp",
        1,
        Engine::WindowParams("TestDesktopApp", Render::USize(1920, 1080)),
        Engine::VROutput::None,
        std::make_unique<DevScene>()
    );

    desktopEngine.Shutdown();

    return 0;
}
