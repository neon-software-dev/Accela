/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SHADERUTIL_H
#define LIBACCELAENGINE_SRC_SHADERUTIL_H

#include <Accela/Platform/File/IFiles.h>
#include <Accela/Render/Shader/ShaderSpec.h>
#include <Accela/Common/Log/ILogger.h>

#include <expected>
#include <vector>
#include <algorithm>
#include <string>
#include <string_view>
#include <optional>

namespace Accela::Engine
{
    [[nodiscard]] static std::optional<Render::ShaderType> ShaderTypeFromFileName(const std::string_view& fileName)
    {
        if (fileName.ends_with(".vert.spv")) return Render::ShaderType::Vertex;
        if (fileName.ends_with(".frag.spv")) return Render::ShaderType::Fragment;
        if (fileName.ends_with(".tesc.spv")) return Render::ShaderType::TESC;
        if (fileName.ends_with(".tese.spv")) return Render::ShaderType::TESE;

        return std::nullopt;
    }

    [[nodiscard]] static std::expected<std::vector<Render::ShaderSpec>, bool>
    ReadShadersFromAssets(const Common::ILogger::Ptr& logger, const Platform::IFiles::Ptr& files)
    {
        //
        // Get list of all file names in the shaders assets subdirectory
        //
        const auto shaderFileNamesExpect = files->ListFilesInAccelaSubdir(Platform::SHADERS_SUBDIR);
        if (!shaderFileNamesExpect)
        {
            return std::unexpected(shaderFileNamesExpect.error());
        }

        //
        // Filter the file names down to those that are compiled SPIR-V (ending in .spv)
        //
        std::vector<std::string> spvFileNames;

        for (const auto& fileName : *shaderFileNamesExpect)
        {
            if (fileName.ends_with(".spv"))
            {
                spvFileNames.push_back(fileName);
            }
        }

        //
        // Transform the SPIR-V filenames into shader specs
        //
        std::vector<Render::ShaderSpec> shaderSpecs;

        for (const auto& spvFileName : spvFileNames)
        {
            const auto shaderTypeOpt = ShaderTypeFromFileName(spvFileName);
            if (!shaderTypeOpt)
            {
                logger->Log(Common::LogLevel::Warning, "ReadShadersFromAssets: Ignoring unknown shader file type: {}", spvFileName);
                continue;
            }

            const auto shaderContentsExpect = files->LoadAccelaFile(Platform::SHADERS_SUBDIR, spvFileName);
            if (!shaderContentsExpect)
            {
                logger->Log(Common::LogLevel::Error, "ReadShadersFromAssets: Failed to load shader contents from file: {}", spvFileName);
                continue;
            }

            shaderSpecs.emplace_back(spvFileName, *shaderTypeOpt, *shaderContentsExpect);
        }

        return shaderSpecs;
    }
}

#endif //LIBACCELAENGINE_SRC_SHADERUTIL_H
