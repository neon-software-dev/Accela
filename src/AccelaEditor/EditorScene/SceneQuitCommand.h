/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_SCENEQUITCOMMAND_H
#define ACCELAEDITOR_EDITORSCENE_SCENEQUITCOMMAND_H

#include "SceneCommand.h"

namespace Accela
{
    class SceneQuitCommand : public SceneCommand
    {
        public:

            static constexpr auto TYPE = "SceneQuitCommand";

        public:

            [[nodiscard]] std::string GetType() const override { return TYPE; };
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_SCENEQUITCOMMAND_H
