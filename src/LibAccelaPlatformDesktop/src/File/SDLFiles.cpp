/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/SDLUtil.h>
#include <Accela/Platform/File/SDLFiles.h>
#include <Accela/Platform/Package/DiskPackageSource.h>

#include <Accela/Common/BuildInfo.h>

#include <SDL2/SDL_filesystem.h>
#include <SDL2/SDL_image.h>

#include <filesystem>
#include <fstream>

namespace Accela::Platform
{

SDLFiles::SDLFiles(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

std::expected<std::vector<std::string>, bool> SDLFiles::ListFilesInDirectory(const std::string& directory) const
{
    std::vector<std::string> filenames;

    try
    {
        for (const auto& dirEntry: std::filesystem::directory_iterator(directory))
        {
            filenames.push_back(dirEntry.path().filename().string());
        }
    }
    catch (const std::exception&)
    {
        m_logger->Log(Common::LogLevel::Error, "ListFilesInDirectory: Failed to read directory: {}", directory);
        return std::unexpected(false);
    }

    return filenames;
}

std::expected<Common::ImageData::Ptr, bool>
SDLFiles::LoadTexture(const std::vector<std::byte>& data,
                      const std::optional<std::string>& dataFormatHint) const
{
    auto pRwOps = SDL_RWFromConstMem((void*)data.data(), (int)data.size());

    SDL_Surface* pSurface{nullptr};

    if (dataFormatHint)
    {
        pSurface = IMG_LoadTyped_RW(pRwOps, 1, dataFormatHint->c_str());
    }
    else
    {
        pSurface = IMG_Load_RW(pRwOps, 1);
    }

    if (pSurface == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error,
          "LoadTexture: IMG_Load failed, had data format? {}, Error: {}", dataFormatHint.has_value(), SDL_GetError());
        return std::unexpected(false);
    }

    auto imageData = SDLUtil::SDLSurfaceToImageData(m_logger, pSurface);
    if (imageData == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadTexture: SDLSurfaceToImageData failed");
        return std::unexpected(false);
    }
    SDL_FreeSurface(pSurface);

    return imageData;
}

std::expected<Common::ImageData::Ptr, unsigned int> SDLFiles::LoadTexture(const std::string& filePath) const
{
    SDL_Surface *pSurface = IMG_Load(filePath.c_str());
    if (pSurface == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadAssetTexture: IMG_Load failed, error: {}", SDL_GetError());
        return std::unexpected(1);
    }

    auto imageData = SDLUtil::SDLSurfaceToImageData(m_logger, pSurface);
    if (imageData == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadAssetTexture: SDLSurfaceToImageData failed");
        return std::unexpected(2);
    }
    SDL_FreeSurface(pSurface);

    return imageData;
}

[[nodiscard]] std::expected<std::vector<unsigned char>, bool> SDLFiles::LoadAccelaFile(const std::string& subdir, const std::string& fileName) const
{
    const auto filePath = GetAccelaFilePath(subdir, fileName);

    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected(false);
    }

    file.seekg(0, std::ios::end);
    std::vector<unsigned char> fileContents(file.tellg());
    file.seekg(0, std::ios::beg);
    fileContents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return fileContents;
}

std::string SDLFiles::GetExecutableDirectory()
{
    auto basePath = SDL_GetBasePath(); // TODO Perf: Documentation says this is a heavy call, only call once and cache
    auto basePathStr = std::string(basePath);
    SDL_free(basePath);

    return basePathStr;
}

std::string SDLFiles::GetAccelaDirectory() const
{
    return GetExecutableDirectory() + EnsureEndsWithSeparator(ACCELA_DIR);
}

std::string SDLFiles::GetAccelaSubdirectory(const std::string& subDirName) const
{
    return GetAccelaDirectory() + EnsureEndsWithSeparator(subDirName);
}

std::string SDLFiles::GetAccelaFilePath(const std::string& subdir, const std::string& fileName) const
{
    return GetAccelaSubdirectory(subdir) + fileName;
}

std::expected<std::vector<std::string>, bool> SDLFiles::ListFilesInAccelaSubdir(const std::string& subdir) const
{
    return ListFilesInDirectory(GetAccelaSubdirectory(subdir));
}

std::string SDLFiles::GetPackagesDirectory() const
{
    return GetAccelaSubdirectory(PACKAGES_DIR);
}

std::string SDLFiles::GetPackageDirectory(const std::string& packageName) const
{
    return GetAccelaSubdirectory(PACKAGES_DIR) + EnsureEndsWithSeparator(packageName);
}

std::expected<PackageSource::Ptr, bool> SDLFiles::LoadPackage(const std::string& packageName) const
{
    const auto packageDirectory = std::filesystem::path(GetPackageDirectory(packageName));
    const auto packageFile = packageDirectory / std::string(packageName + Platform::PACKAGE_EXTENSION);

    const auto package = DiskPackageSource::OpenOnDisk(packageFile);
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error,
          "SDLFiles::LoadPackage: Failed to load package {}, error code: {}", packageName, (int)package.error());
        return std::unexpected(false);
    }

    return *package;
}

std::string SDLFiles::GetSubdirPath(const std::string& root, const std::string& subdir) const
{
    return EnsureEndsWithSeparator(EnsureEndsWithSeparator(root) + subdir);
}

std::string SDLFiles::EnsureEndsWithSeparator(const std::string& source) const
{
    const char pathSeparator = std::filesystem::path::preferred_separator;

    if (source.empty())
    {
        return std::string{pathSeparator};
    }

    const bool endsWithSeparator = source.at(source.length() - 1) == pathSeparator;
    if (endsWithSeparator)
    {
        return source;
    }
    else
    {
        return source + pathSeparator;
    }
}

}
