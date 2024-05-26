#ifndef LIBACCELAENGINE_SRC_AUDIO_AUDIOUTIL_H
#define LIBACCELAENGINE_SRC_AUDIO_AUDIOUTIL_H

// Must be included before AudioFile.h as there's a bug in AudioFile lib
// where it'll fail to compile on newer C++ versions otherwise
#include <cstdint>

#include <AudioFile.h>

#include <vector>
#include <cstddef>

namespace Accela::Engine
{
    struct AudioUtil
    {
        /**
         * Converts an AudioFile to a vector of bytes which represent the audio file
         */
        static std::vector<std::byte> AudioFileToByteBuffer(const AudioFile<double>& audioFile);
    };
}

#endif //LIBACCELAENGINE_SRC_AUDIO_AUDIOUTIL_H
