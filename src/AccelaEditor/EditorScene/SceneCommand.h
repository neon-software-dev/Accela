/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_SCENECOMMAND_H
#define ACCELAEDITOR_EDITORSCENE_SCENECOMMAND_H

#include <memory>
#include <string>

namespace Accela
{
    class SceneCommand
    {
        public:

            using Ptr = std::shared_ptr<SceneCommand>;

        public:

            virtual ~SceneCommand() = default;

            [[nodiscard]] virtual std::string GetType() const = 0;
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_SCENECOMMAND_H
