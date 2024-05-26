#include "AssimpLogStream.h"

namespace Accela::Engine
{

AssimpLogStream* AssimpLogStream::pAssimpLogStream = nullptr;

static const unsigned int AssimpLogSeverity = Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;

void AssimpLogStream::Install(const Common::ILogger::Ptr& logger)
{
    Assimp::DefaultLogger::create("DefaultLogger", Assimp::Logger::NORMAL, aiDefaultLogStream_DEBUGGER);
    pAssimpLogStream = new AssimpLogStream(logger);
    Assimp::DefaultLogger::get()->attachStream(pAssimpLogStream, AssimpLogSeverity);
}

void AssimpLogStream::Uninstall()
{
    Assimp::DefaultLogger::get()->detachStream(pAssimpLogStream, AssimpLogSeverity);
    Assimp::DefaultLogger::kill();

    delete(pAssimpLogStream);
    pAssimpLogStream = nullptr;
}

}
