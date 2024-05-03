/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_METRICS_H
#define LIBACCELAENGINE_SRC_METRICS_H

namespace Accela::Engine
{
    // Engine general
    static constexpr char Engine_SimulationStep_Time[] = "Engine_SimulationStep_Time";
    static constexpr char Engine_SceneSimulationStep_Time[] = "Engine_SceneSimulationStep_Time";

    // RendererSyncSystem
    static constexpr char Engine_RendererSyncSystem_Time[] = "Engine_RendererSyncSystem_Time";

    // PhysicsSyncSystem
    static constexpr char Engine_PhysicsSyncSystem_Time[] = "Engine_PhysicsSyncSystem_Time";
    static constexpr char Engine_Physics_Internal_Bodies_Count[] = "Engine_Physics_Internal_Bodies_Count";
    static constexpr char Engine_Physics_Static_Bodies_Count[] = "Engine_Physics_Static_Bodies_Count";
    static constexpr char Engine_Dynamic_Rigid_Bodies_Count[] = "Engine_Dynamic_Rigid_Bodies_Count";
}

#endif //LIBACCELAENGINE_SRC_METRICS_H
