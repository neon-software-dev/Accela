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

void WrappedSceneCallbacks::OnSceneStop()
{
    m_pWrappedCallbacks->OnSceneStop();
}

void WrappedSceneCallbacks::OnSimulationStep(unsigned int timeStep)
{
    m_pWrappedCallbacks->OnSimulationStep(timeStep);
}

void WrappedSceneCallbacks::OnKeyEvent(const Platform::KeyEvent& event)
{
    m_pWrappedCallbacks->OnKeyEvent(event);
}

void WrappedSceneCallbacks::OnMouseMoveEvent(const Platform::MouseMoveEvent& event)
{
    m_pWrappedCallbacks->OnMouseMoveEvent(event);
}

void WrappedSceneCallbacks::OnMouseButtonEvent(const Platform::MouseButtonEvent& event)
{
    m_pWrappedCallbacks->OnMouseButtonEvent(event);
}

}
