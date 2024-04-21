/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "EngineAssets.h"

#include "Audio/AudioUtil.h"

namespace Accela::Engine
{

EngineAssets::EngineAssets(Common::ILogger::Ptr logger, Platform::IFiles::Ptr files)
    : m_logger(std::move(logger))
    , m_files(std::move(files))
    , m_modelLoader(m_logger, m_files)
{

}

std::expected<TextureData, bool> EngineAssets::ReadTextureBlocking(const std::string& textureName) const
{
    const auto resultExpected = m_files->LoadAssetTexture(textureName);
    if (!resultExpected)
    {
        m_logger->Log(Common::LogLevel::Error, "ReadTexture: Failed to load texture file from disk: {}", textureName);
        return std::unexpected(false);
    }

    return TextureData(resultExpected.value());
}

std::expected<TextureData, bool> EngineAssets::ReadCubeTextureBlocking(const std::array<std::string, 6>& textureNames) const
{
    TextureData textureData{};

    for (const auto& textureName : textureNames)
    {
        const auto resultExpected = m_files->LoadAssetTexture(textureName);
        if (!resultExpected)
        {
            m_logger->Log(Common::LogLevel::Error, "ReadTexture: Failed to load texture file from disk: {}", textureName);
            return std::unexpected(false);
        }

        textureData.textureImages.push_back(*resultExpected);
    }

    return textureData;
}

std::expected<Common::AudioData::Ptr, bool> EngineAssets::ReadAudioBlocking(const std::string& audioName) const
{
    auto fileContentsExpected = m_files->LoadAssetFile(Platform::AUDIO_SUBDIR, audioName);
    if (!fileContentsExpected)
    {
        m_logger->Log(Common::LogLevel::Error, "ReadAudio: Failed to load audio file from disk: {}", audioName);
        return std::unexpected(false);
    }

    AudioFile<double> audioFile;
    if (!audioFile.loadFromMemory(fileContentsExpected.value()))
    {
        m_logger->Log(Common::LogLevel::Error, "ReadAudio: Failed to load audio file from memory: {}", audioName);
        return std::unexpected(false);
    }

    Common::AudioData::Format audioFileFormat{};

    if (audioFile.getNumChannels() == 1 && audioFile.getBitDepth() == 8)
    {
        audioFileFormat = Common::AudioData::Format::Mono8;
    }
    else if (audioFile.getNumChannels() == 1)
    {
        // We transform all bit depth >= 16 to 16 bit as that's the most OpenAL supports
        audioFileFormat = Common::AudioData::Format::Mono16;
    }
    else if (audioFile.getNumChannels() == 2 && audioFile.getBitDepth() == 8)
    {
        audioFileFormat = Common::AudioData::Format::Stereo8;
    }
    else if (audioFile.getNumChannels() == 2)
    {
        // We transform all bit depth >= 16 to 16 bit as that's the most OpenAL supports
        audioFileFormat = Common::AudioData::Format::Stereo16;
    }
    else
    {
        m_logger->Log(Common::LogLevel::Error,
            "ReadAudio: Unsupported audio file: {}. Num channels: {}, bit depth: {}",
            audioName,
            audioFile.getNumChannels(),
            audioFile.getBitDepth()
        );
        return std::unexpected(false);
    }

    std::vector<std::byte> audioByteBuffer = AudioUtil::AudioFileToByteBuffer(audioFile);

    return std::make_shared<Common::AudioData>(
        audioFileFormat,
        audioFile.getSampleRate(),
        std::move(audioByteBuffer)
    );
}

std::expected<Model::Ptr, bool>
EngineAssets::ReadModelBlocking(const std::string& modelName, const std::string& modelExtension) const
{
    const auto modelsDirectory = m_files->GetAssetsSubdirectory(Platform::MODELS_DIR);
    const auto modelDirectory = m_files->GetSubdirPath(modelsDirectory, modelName);
    const auto modelFilePath = modelDirectory + modelName + modelExtension;

    auto model = m_modelLoader.LoadModel(modelFilePath);
    if (model == nullptr)
    {
        return std::unexpected(false);
    }

    return model;
}

}
