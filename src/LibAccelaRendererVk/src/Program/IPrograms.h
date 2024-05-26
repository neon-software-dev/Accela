/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_PROGRAM_IPROGRAMS
#define LIBACCELARENDERERVK_SRC_PROGRAM_IPROGRAMS

#include "ProgramDef.h"

#include "../ForwardDeclares.h"

#include <string>
#include <vector>

namespace Accela::Render
{
    /**
     * System which manages shader programs
     */
    class IPrograms
    {
        public:

            virtual ~IPrograms() = default;

            /**
             * Destroys all previously created programs
             */
            virtual void Destroy() = 0;

            /**
             * Create a program with the provided program definition
             *
             * @return True if the program already existed or was successfully created
             */
            virtual bool CreateProgram(const std::string& programName, const std::vector<std::string>& shaders) = 0;

            /**
             * Retrieve the details of a previously loaded program
             *
             * @return The program's details or nullptr if no such program
             */
            [[nodiscard]] virtual ProgramDefPtr GetProgramDef(const std::string& programName) const = 0;

            /**
             * Release all resources associated with the specified program
             */
            virtual void DestroyProgram(const std::string& programName) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_PROGRAM_IPROGRAMS
