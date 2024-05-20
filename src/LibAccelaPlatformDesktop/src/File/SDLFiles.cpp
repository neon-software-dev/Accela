/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/File/SDLFiles.h>

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

std::expected<std::vector<std::string>, bool> SDLFiles::ListFilesInAssetsSubdir(const std::string& subdir) const
{
    return ListFilesInDirectory(GetAssetsSubdirectory(subdir));
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

std::expected<Common::ImageData::Ptr, unsigned int> SDLFiles::LoadAssetTexture(const std::string& fileName) const
{
    const auto filePath = GetAssetFilePath(TEXTURES_SUBDIR, fileName);

    return LoadTexture(filePath);
}

std::expected<Common::ImageData::Ptr, bool>
SDLFiles::LoadCompressedTexture(const std::vector<std::byte>& data,
                                const size_t& dataByteSize,
                                const std::optional<std::string>& dataFormatHint) const
{
    auto pRwOps = SDL_RWFromMem((void*)data.data(), (int)dataByteSize);

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
        m_logger->Log(Common::LogLevel::Error, "LoadCompressedTexture: IMG_Load failed, had data format? {}", dataFormatHint.has_value());
        return std::unexpected(false);
    }

    auto imageData = SDLSurfaceToImageData(m_logger, pSurface);
    if (imageData == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadCompressedTexture: SDLSurfaceToImageData failed");
        return std::unexpected(false);
    }

    return imageData;
}

std::expected<Common::ImageData::Ptr, unsigned int>
SDLFiles::LoadAssetModelTexture(const std::string& modelName, const std::string& fileName) const
{
    const auto modelsDir = GetAssetsSubdirectory(MODELS_DIR);
    const auto modelDir = GetSubdirPath(modelsDir, modelName);

    return LoadTexture(modelDir + fileName);
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

[[nodiscard]] std::expected<std::vector<unsigned char>, bool> SDLFiles::LoadAssetFile(const std::string& subdir, const std::string& fileName) const
{
    const auto filePath = GetAssetFilePath(subdir, fileName);

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

std::string SDLFiles::GetAssetsDirectory() const
{
    return GetExecutableDirectory() + EnsureEndsWithSeparator(ASSETS_DIR);
}

std::string SDLFiles::GetAssetsSubdirectory(const std::string& subDirName) const
{
    return GetAssetsDirectory() + EnsureEndsWithSeparator(subDirName);
}

std::string SDLFiles::GetAssetFilePath(const std::string& subdir, const std::string& fileName) const
{
    return GetAssetsSubdirectory(subdir) + fileName;
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
