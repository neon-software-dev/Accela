/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PacketDecoder.h"

namespace Accela::Engine
{

template <>
std::vector<VideoFrame> PacketDecoder<VideoFrame>::Thread_ReceiveFramesFromDecoder()
{
    //Common::Timer t("ReceiveVideoFramesFromDecoder");
    auto result = m_pContainer->ReceiveVideoFramesFromDecoder();
    //t.StopTimer(m_logger);
    return result;
}

template <>
std::vector<AudioFrame> PacketDecoder<AudioFrame>::Thread_ReceiveFramesFromDecoder()
{
    //Common::Timer t("ReceiveAudioFramesFromDecoder");
    auto result = m_pContainer->ReceiveAudioFramesFromDecoder();
    //t.StopTimer(m_logger);
    return result;
}

}
