/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
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

    auto imageData = SDLSurfaceToImageData(m_logger, pSurface);
    if (imageData == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadTexture: SDLSurfaceToImageData failed");
        return std::unexpected(false);
    }

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

    auto imageData = SDLSurfaceToImageData(m_logger, pSurface);
    if (imageData == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadAssetTexture: SDLSurfaceToImageData failed");
        return std::unexpected(2);
    }

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
    return SDL_GetBasePath();
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
    std::string pathSeparator;

    const Common::OS os = Common::BuildInfo::GetOS();
    switch (os)
    {
        case Common::OS::Windows:
            pathSeparator = "\\";
        break;
        default:
            pathSeparator = "/";
        break;
    }

    if (source.length() < pathSeparator.length())
    {
        return source;
    }

    const bool endsWithSeparator = source.compare(source.length() - pathSeparator.length(), pathSeparator.length(), pathSeparator) == 0;
    if (endsWithSeparator)
    {
        return source;
    }
    else
    {
        return source + pathSeparator;
    }
}

Common::ImageData::Ptr SDLFiles::SDLSurfaceToImageData(const Common::ILogger::Ptr& logger, SDL_Surface *pSurface)
{
    SDL_LockSurface(pSurface);

    SDL_Surface* pFormattedSurface = nullptr;

    if (pSurface->format->format == SDL_PIXELFORMAT_RGBA32)
    {
        pFormattedSurface = pSurface;
    }
    else
    {
        // Convert the surface to RGBA32 as that's what the Renderer wants for textures
        pFormattedSurface = SDL_ConvertSurfaceFormat(pSurface, SDL_PIXELFORMAT_RGBA32, 0);

        // Unlock the old surface
        SDL_UnlockSurface(pSurface);

        if (pFormattedSurface == nullptr)
        {
            logger->Log(Common::LogLevel::Error,
            "SDLSurfaceToImageData: Surface could not be converted to a supported pixel format");
            return nullptr;
        }

        // Lock the new surface for reading its pixels
        SDL_LockSurface(pFormattedSurface);
    }

    // Byte size of the pixel data
    const uint32_t pixelDataByteSize = pFormattedSurface->w * pFormattedSurface->h * pFormattedSurface->format->BytesPerPixel;

    // Copy the surface's pixel data into a vector
    std::vector<std::byte> imageBytes;
    imageBytes.reserve(pixelDataByteSize);
    imageBytes.insert(imageBytes.end(), (std::byte*)pFormattedSurface->pixels, (std::byte*)pFormattedSurface->pixels + pixelDataByteSize);

    Common::ImageData::Ptr loadResult = std::make_shared<Common::ImageData>(
        imageBytes,
        1,
        static_cast<unsigned int>(pFormattedSurface->w),
        static_cast<unsigned int>(pFormattedSurface->h),
        Common::ImageData::PixelFormat::RGBA32
    );

    SDL_UnlockSurface(pFormattedSurface);
    SDL_FreeSurface(pSurface);

    return loadResult;
}

}
