#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEYEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEYEVENT_H

#include "Key.h"

namespace Accela::Platform
{
    /**
     * Represents a keyboard key event, as reported by the OS
     */
    struct KeyEvent
    {
        enum class Action
        {
            KeyPress,
            KeyRelease
        };

        KeyEvent(Action _action, Key _key)
            : action(_action)
            , key(_key)
        { }

        // Whether the key was pressed or released
        Action action;

        // The key in question
        Key key;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEYEVENT_H
