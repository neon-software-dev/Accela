#ifndef LIBACCELAENGINE_SRC_PHYSICS_PHYSXLOGGER_H
#define LIBACCELAENGINE_SRC_PHYSICS_PHYSXLOGGER_H

#include "PhysXWrapper.h"

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Engine
{
    struct PhysxLogger : public physx::PxErrorCallback
    {
        explicit PhysxLogger(Common::ILogger::Ptr logger)
            : m_logger(std::move(logger))
        { }

        void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
        {
            Common::LogLevel logLevel{Common::LogLevel::Error};

            switch (code)
            {
                case physx::PxErrorCode::eNO_ERROR:
                case physx::PxErrorCode::eDEBUG_INFO:
                case physx::PxErrorCode::eMASK_ALL:
                    logLevel = Common::LogLevel::Info;
                    break;
                case physx::PxErrorCode::eDEBUG_WARNING:
                case physx::PxErrorCode::ePERF_WARNING:
                    logLevel = Common::LogLevel::Warning;
                    break;
                case physx::PxErrorCode::eINVALID_PARAMETER:
                case physx::PxErrorCode::eINVALID_OPERATION:
                case physx::PxErrorCode::eOUT_OF_MEMORY:
                case physx::PxErrorCode::eINTERNAL_ERROR:
                    logLevel = Common::LogLevel::Error;
                    break;
                case physx::PxErrorCode::eABORT:
                    logLevel = Common::LogLevel::Fatal;
                    break;
            }

            m_logger->Log(logLevel, "[PhysX] {} - {} , {}", message, file, line);
        }

        Common::ILogger::Ptr m_logger;
    };
}

#endif //LIBACCELAENGINE_SRC_PHYSICS_PHYSXLOGGER_H
