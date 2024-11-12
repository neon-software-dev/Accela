/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGCONTAINER_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGCONTAINER_H

#include "SWRConfig.h"
#include "FilterGraphConfig.h"

#include "../MediaCommon.h"

#include <Accela/Common/Log/ILogger.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
    #include <libavfilter/avfilter.h>
}

#include <expected>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Accela::Engine
{
    /**
     * Wrapper around FFMPEG objects. Initializes and manages an FFMPEG instance which
     * allows reading/decoding from an FFMPEG source.
     */
    class FFMPEGContainer
    {
        public:

            using Ptr = std::shared_ptr<FFMPEGContainer>;
            using UPtr = std::unique_ptr<FFMPEGContainer>;

            struct Config
            {
                Common::AudioDataFormat audioOutputFormat{Common::AudioDataFormat::Stereo16};
            };

            enum class ReadException
            {
                Eof,    // EOF, no packets left to be read
                Error   // Legitimate error while reading from the container
            };

            enum class SendPacketException
            {
                Full,   // The decoder is full and not accepting more packets
                Error   // Legitimate error while sending packets to the decoder
            };

        public:

            FFMPEGContainer(Common::ILogger::Ptr logger, Config config);
            ~FFMPEGContainer();

            [[nodiscard]] bool Open(const std::string& url);
            [[nodiscard]] bool LoadBestStreams();
            [[nodiscard]] bool LoadStreams(const std::unordered_set<unsigned int>& streamIndices);
            void Destroy();

            [[nodiscard]] MediaDuration GetSourceDuration() const;
            [[nodiscard]] std::optional<std::pair<unsigned int, unsigned int>> GetVideoStreamDimensions() const;
            [[nodiscard]] bool IsEOF() const { return m_eof; }
            [[nodiscard]] int GetVideoStreamIndex() const { return m_videoStreamIndex; }
            [[nodiscard]] int GetAudioStreamIndex() const { return m_audioStreamIndex; }
            [[nodiscard]] std::optional<SubtitleSource> GetActiveSubtitleSource() const { return m_subtitleSource; }

            [[nodiscard]] std::expected<AVPacket*, ReadException> ReadPacket();

            [[nodiscard]] std::optional<SendPacketException> SendPacketToDecoder(MediaStreamType mediaStreamType, const AVPacket* pPacket);
            [[nodiscard]] std::vector<VideoFrame> ReceiveVideoFramesFromDecoder();
            [[nodiscard]] std::vector<AudioFrame> ReceiveAudioFramesFromDecoder();
            [[nodiscard]] std::optional<SubtitleFrame> DecodeSubtitle(const AVPacket* pPacket);

            [[nodiscard]] bool SeekToPoint(const MediaPoint& point, const std::optional<MediaDuration>& relative = std::nullopt);

            void SetAudioSyncDiff(const MediaDuration& audioSyncDiff) { m_audioSyncDiff = audioSyncDiff; }
            void FlushDecoder(MediaStreamType mediaStreamType);

        private:

            struct FFMPEGStream
            {
                int streamIndex{-1};
                const AVStream* pStream{nullptr};
                const AVCodec* pCodec{nullptr};
                AVCodecContext* pCodecContext{nullptr};

                // Specific to video streams
                std::optional<FilterGraphConfig> filterGraphConfig;
                AVFilterInOut* pAVFilterInputs{nullptr};
                AVFilterInOut* pAVFilterOutputs{nullptr};
                AVFilterContext* pAVFilterBufferSinkCtx{nullptr};
                AVFilterContext* pAVFilterBufferSrcCtx{nullptr};
                AVFilterGraph* pAVFilterGraph{nullptr};

                // Specific to audio streams
                std::optional<SWRConfig> swrConfig;
                SwrContext* swrContext{nullptr};

                // Specific to hardware-decoded video streams
                std::unique_ptr<AVPixelFormat> pHWAVPixelFormat;
                AVBufferRef* pHWDeviceContext{nullptr};

                // Persistent work buffers
                AVFrame* pFrame{nullptr};
                AVFrame* pHWDestFrame{nullptr};
                AVFrame* pFiltFrame{nullptr};
            };

            enum class ReceiveFrameException
            {
                Error,
                Dry,
                Eof
            };

        private:

            [[nodiscard]] bool LoadAVStream(const StreamInfo& streamInfo);
            [[nodiscard]] bool LoadSubtitleStream(const StreamInfo& streamInfo);

            [[nodiscard]] std::expected<AVFormatContext*, bool> OpenFormat(const std::string& url);
            [[nodiscard]] std::unordered_map<unsigned int, StreamInfo> FetchStreamInfo();

            [[nodiscard]] std::expected<std::unique_ptr<FFMPEGStream>, bool> OpenAVStream(
                AVFormatContext* pAVFormatContext,
                unsigned int streamIndex,
                bool supportMultiThreadedDecode,
                bool supportHardwareDecode);

            [[nodiscard]] int MediaStreamTypeToStreamIndex(MediaStreamType mediaStreamType) const;

            void ActivateSubtitle(const SubtitleSource& subtitleSource);

            [[nodiscard]] std::optional<ReceiveFrameException> ReceiveFrameFromDecoder(FFMPEGStream* pStream, AVFrame* pOutput);
            [[nodiscard]] bool InsertAVFramesIntoFilterGraph(FFMPEGStream* pStream, const std::vector<AVFrame*>& frames);
            [[nodiscard]] std::vector<Common::ImageData::Ptr> ReceiveImageDatasFromFilterGraph(FFMPEGStream* pStream);

            [[nodiscard]] int GetSyncAdjustedNumSamples(int frameNumSamples, int frameSampleRate);
            [[nodiscard]] std::expected<Common::AudioData::Ptr, bool> ConvertAVFrameToAudio(FFMPEGStream* pStream, AVFrame* pFrame);

            [[nodiscard]] FilterGraphConfig GetFilterGraphConfig(FFMPEGStream* pVideoStream,
                                                                 AVPixelFormat destPixelFormat,
                                                                 const std::optional<SubtitleSource>& subtitleSource) const;
            [[nodiscard]] bool CreateVideoFilterGraphAsNeeded(FFMPEGStream* pVideoStream,
                                                              AVPixelFormat destPixelFormat,
                                                              const std::optional<SubtitleSource>& subtitleSource);

            [[nodiscard]] bool CreateAudioSwrAsNeeded(FFMPEGStream* pStream,
                                                      const AVChannelLayout& destChannelLayout,
                                                      AVSampleFormat destSampleFormat,
                                                      int destSampleRate);

            static void DestroyStreamObjects(FFMPEGStream* pStream);
            static void DestroySWRObjects(FFMPEGStream* pStream);
            static void DestroyFilterGraphObjects(FFMPEGStream* pStream);

        private:

            Common::ILogger::Ptr m_logger;
            Config m_config;

            std::string m_url;
            AVFormatContext* m_pFormatContext{nullptr};
            std::unordered_map<unsigned int, StreamInfo> m_streamInfos;

            // Stream objects
            std::unordered_map<unsigned int, std::unique_ptr<FFMPEGStream>> m_openStreams;
            int m_videoStreamIndex{-1};
            int m_audioStreamIndex{-1};

            // Pre-allocated work buffers
            AVPacket* m_pPacket{nullptr};

            bool m_eof{false};

            MediaDuration m_audioSyncDiff{0.0};

            std::optional<SubtitleSource> m_subtitleSource;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGCONTAINER_H
