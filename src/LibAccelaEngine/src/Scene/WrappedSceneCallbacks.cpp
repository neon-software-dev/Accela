/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WrappedSceneCallbacks.h"

namespace Accela::Engine
{

WrappedSceneCallbacks::WrappedSceneCallbacks(SceneCallbacks *pWrappedCalls)
    : m_pWrappedCallbacks(pWrappedCalls)
{

}

void WrappedSceneCallbacks::OnSceneStart(const IEngineRuntime::Ptr& engine)
{
    m_pWrappedCallbacks->OnSceneStart(engine);
}

void WrappedSceneCallbacks::OnSceneStop(const IEngineRuntime::Ptr& engine)
{
    m_pWrappedCallbacks->OnSceneStop(engine);
}

void WrappedSceneCallbacks::OnSimulationStep(const IEngineRuntime::Ptr& engine, unsigned int timeStep)
{
    m_pWrappedCallbacks->OnSimulationStep(engine, timeStep);
}

void WrappedSceneCallbacks::OnKeyEvent(const IEngineRuntime::Ptr& engine, const Platform::KeyEvent& event)
{
    m_pWrappedCallbacks->OnKeyEvent(engine, event);
}

void WrappedSceneCallbacks::OnMouseMoveEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseMoveEvent& event)
{
    m_pWrappedCallbacks->OnMouseMoveEvent(engine, event);
}

void WrappedSceneCallbacks::OnMouseButtonEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseButtonEvent& event)
{
    m_pWrappedCallbacks->OnMouseButtonEvent(engine, event);
}

}
