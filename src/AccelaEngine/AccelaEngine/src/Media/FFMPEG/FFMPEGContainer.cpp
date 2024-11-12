/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FFMPEGContainer.h"
#include "FFMPEGCommon.h"

#include <Accela/Common/Thread/ResultMessage.h>
#include <Accela/Common/Timer.h>

#include <algorithm>
#include <functional>

extern "C"
{
    #include <libavutil/opt.h>
    #include <libavfilter/buffersrc.h>
    #include <libavfilter/buffersink.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/avstring.h>
}

/*
 * TODO!
 * ./vcpkg install ffmpeg[core,avcodec,avdevice,avformat,avfilter,ass,swresample,nvcodec]:x64-linux-dynamic --overlay-triplets=custom-triplets
 * ./vcpkg install ffmpeg[core,avcodec,avdevice,avformat,avfilter,ass,drawtext,fontconfig,freetype,ffplay,swresample,nvcodec]:x64-linux-dynamic --overlay-triplets=custom-triplets
 gnutls
 */

// TODO! Valgrind check for leaks - filter add/remove frames leak?
// TODO! Verify that video/audio stream queue lengths dont get too big
// TODO! add hard delays into decoding to test slow device
// TODO! av_log_set_callback, pass through logger
// TODO! Hardware decode fails with impact.mp4
// TODO! Config for whether to do thread based or slice based multi-threading (best guess based on codec?)
// TODO! vaapi filter graph filter for gpu scaling?
// TODO! Evaluate which av includes are really needed
// TODO! FFMPEG license info
// TODO! Dont use audio master clock if no audio stream, default to audio clock(or external?)
// TODO! ass - subtitles filter - original_size , other params
// TODO! Audio manager thread safety
// TODO! global vs local media audio
// TODO! - video with quotes in name broke
// TODO! - Videos with "non-text subtitles" - are these the graphic/image ones? Read and process their packets?

namespace Accela::Engine
{

// No audio sync compensation if the audio is out of sync by less than a minimum amount
static constexpr auto MIN_SYNC_ADJUSTMENT_LEVEL = std::chrono::duration_cast<MediaDuration>(
    std::chrono::milliseconds(5));

static enum AVPixelFormat GetHWFormat(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const AVPixelFormat *pHWPixelFormat = (AVPixelFormat *) ctx->opaque;
    if (pHWPixelFormat == nullptr)
    {
        return AV_PIX_FMT_NONE;
    }

    for (auto pix_fmt = pix_fmts; *pix_fmt != -1; ++pix_fmt)
    {
        if (*pix_fmt == *pHWPixelFormat)
        {
            return *pix_fmt;
        }
    }

    return AV_PIX_FMT_NONE;
}

FFMPEGContainer::FFMPEGContainer(Common::ILogger::Ptr logger, Config config)
    : m_logger(std::move(logger))
    , m_config(config)
{

}

FFMPEGContainer::~FFMPEGContainer()
{
    Destroy();
}

bool FFMPEGContainer::Open(const std::string& url)
{
    LogInfo("FFMPEGContainer: Creating video source from URL: {}", url);

    //
    // Destroy any previous resources
    //
    Destroy();

    //
    // Open the URL as an AVFormatContext
    //
    const auto avFormatContext = OpenFormat(url);
    if (!avFormatContext)
    {
        LogError("FFMPEGContainer::Open: Failed to open URL: {}", url);
        Destroy();
        return false;
    }

    m_pFormatContext = *avFormatContext;

    //
    // Read info about the container's streams
    //
    m_streamInfos = FetchStreamInfo();

    //
    // Pre-allocate memory for fetching data
    //
    m_pPacket = av_packet_alloc();
    if (m_pPacket == nullptr)
    {
        LogError("FFMPEGContainer::Open: av_packet_alloc failed");
        Destroy();
        return false;
    }

    m_url = url;

    return true;
}

std::expected<AVFormatContext *, bool> FFMPEGContainer::OpenFormat(const std::string& url)
{
    AVFormatContext *pFormatContext{nullptr};

    //
    // Open the URL
    //
    if (const int result = avformat_open_input(&pFormatContext, url.c_str(), nullptr, nullptr); result != 0)
    {
        LogError("FFMPEGContainer::OpenFormat: avformat_open_input failed, error: {}", AVErrorStr(result));
        return std::unexpected(false);
    }

    //
    // Read in the container's stream metadata
    //
    if (const int result = avformat_find_stream_info(pFormatContext, nullptr); result < 0)
    {
        LogError("FFMPEGContainer::OpenFormat: avformat_find_stream_info failed, error: {}", AVErrorStr(result));
        avformat_close_input(&pFormatContext);
        return std::unexpected(false);
    }

    return pFormatContext;
}

std::unordered_map<unsigned int, StreamInfo> FFMPEGContainer::FetchStreamInfo()
{
    LogInfo("--- FFMPEG Container Stream Infos ---");

    std::unordered_map<unsigned int, StreamInfo> streamInfos;

    unsigned int subtitleIndex = 0;

    for (unsigned int streamIndex = 0; streamIndex < m_pFormatContext->nb_streams; ++streamIndex)
    {
        LogInfo("== Stream Index {} ==", streamIndex);

        const auto pStream = m_pFormatContext->streams[streamIndex];

        MediaStreamType mediaStreamType{MediaStreamType::Video};

        const auto codecType = pStream->codecpar->codec_type;

        if (codecType == AVMediaType::AVMEDIA_TYPE_VIDEO) { mediaStreamType = MediaStreamType::Video; }
        else if (codecType == AVMediaType::AVMEDIA_TYPE_AUDIO) { mediaStreamType = MediaStreamType::Audio; }
        else if (codecType == AVMediaType::AVMEDIA_TYPE_SUBTITLE) { mediaStreamType = MediaStreamType::Subtitle; }
        else
        {
            LogInfo("Unsupported codec type: {}", (int) codecType);
            LogInfo("");
            continue;
        }

        StreamInfo streamInfo(mediaStreamType, streamIndex);

        if (mediaStreamType == MediaStreamType::Subtitle)
        {
            streamInfo.subtitleIndex = subtitleIndex++;
        }

        if (pStream->metadata != nullptr)
        {
            const AVDictionaryEntry *entry = nullptr;
            entry = av_dict_iterate(pStream->metadata, entry);

            while (entry != nullptr)
            {
                streamInfo.metadata.insert({entry->key, entry->value});
                entry = av_dict_iterate(pStream->metadata, entry);
            }
        }

        streamInfo.codecID = pStream->codecpar->codec_id;
        streamInfo.codecName = avcodec_get_name(pStream->codecpar->codec_id);

        streamInfos.insert({streamInfo.streamIndex, streamInfo});

        LogInfo("Type: {}, Codec: {}", TagForMediaStreamType(streamInfo.streamType), streamInfo.codecName);
        LogInfo("Metadata:");
        for (const auto& it: streamInfo.metadata)
        {
            LogInfo("{}, {}", it.first, it.second);
        }

        LogInfo("");
    }

    return streamInfos;
}

bool FFMPEGContainer::LoadBestStreams()
{
    std::unordered_set<unsigned int> bestStreamIndices;

    //
    // Query FFMPEG for best stream indices
    //
    const auto videoBestStreamIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoBestStreamIndex < 0)
    {
        LogError("FFMPEGContainer::LoadBestStreams: Failed to find best video stream, error: {}", AVErrorStr(videoBestStreamIndex));
        return false;
    }

    bestStreamIndices.insert(videoBestStreamIndex);

    const auto audioBestStreamIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioBestStreamIndex < 0)
    {
        LogError("FFMPEGContainer::LoadBestStreams: Failed to find best audio stream, error: {}", AVErrorStr(audioBestStreamIndex));
        return false;
    }

    bestStreamIndices.insert(audioBestStreamIndex);

    const auto subtitleBestStreamIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);
    if (subtitleBestStreamIndex >= 0)
    {
        bestStreamIndices.insert(subtitleBestStreamIndex);
    }

    //
    // Load the best streams
    //
    return LoadStreams(bestStreamIndices);
}

bool FFMPEGContainer::LoadStreams(const std::unordered_set<unsigned int>& streamIndices)
{
    bool allSuccessful = true;

    for (const unsigned int& streamIndex: streamIndices)
    {
        LogInfo("FFMPEGContainer: Loading stream {}", streamIndex);

        if (m_openStreams.contains(streamIndex))
        {
            LogInfo("FFMPEGContainer::LoadStreams: Stream is already open: {}", streamIndex);
            continue;
        }

        const auto streamInfoIt = m_streamInfos.find(streamIndex);
        if (streamInfoIt == m_streamInfos.cend())
        {
            LogError("FFMPEGContainer::LoadStreams: No stream info exists for index: {}", streamIndex);
            allSuccessful = false;
            continue;
        }

        switch (streamInfoIt->second.streamType)
        {
            case MediaStreamType::Video:
            case MediaStreamType::Audio:
            {
                if (!LoadAVStream(streamInfoIt->second))
                {
                    LogError("FFMPEGContainer::LoadStreams: Error loading AV stream: {}", streamIndex);
                    allSuccessful = false;
                }
            }
            break;
            case MediaStreamType::Subtitle:
            {
                if (!LoadSubtitleStream(streamInfoIt->second))
                {
                    LogError("FFMPEGContainer::LoadStreams: Error loading subtitle stream: {}", streamIndex);
                    allSuccessful = false;
                }
            }
            break;
        }
    }

    return allSuccessful;
}

bool FFMPEGContainer::LoadAVStream(const StreamInfo& streamInfo)
{
    auto stream = OpenAVStream(m_pFormatContext, streamInfo.streamIndex, true, false);
    if (!stream)
    {
        LogError("FFMPEGContainer::LoadStreams: Failed to open stream: {}", streamInfo.streamIndex);
        return false;
    }

    if (streamInfo.streamType == MediaStreamType::Video)
    {
        // Destroy any previously loaded video stream
        if (m_videoStreamIndex != -1)
        {
            LogInfo("FFMPEGContainer::LoadAVStream: Destroying previously loaded video stream: {}", m_videoStreamIndex);

            // Flush decoder of old data
            FlushDecoder(MediaStreamType::Video);

            // Destroy stream objects
            DestroyStreamObjects(m_openStreams.at(m_videoStreamIndex).get());
            m_openStreams.erase(m_videoStreamIndex);
        }

        // Record the new video stream
        m_videoStreamIndex = (int)streamInfo.streamIndex;
        m_openStreams.insert({streamInfo.streamIndex, std::move(*stream)});
    }
    else if (streamInfo.streamType == MediaStreamType::Audio)
    {
        // Destroy any previously loaded audio stream
        if (m_audioStreamIndex != -1)
        {
            LogInfo("FFMPEGContainer::LoadAVStream: Destroying previously loaded audio stream: {}", m_audioStreamIndex);

            // Flush decoder of old data
            FlushDecoder(MediaStreamType::Audio);

            // Destroy stream objects
            DestroyStreamObjects(m_openStreams.at(m_audioStreamIndex).get());
            m_openStreams.erase(m_audioStreamIndex);
        }

        // Record the new audio stream
        m_audioStreamIndex = (int)streamInfo.streamIndex;
        m_openStreams.insert({(int)streamInfo.streamIndex, std::move(*stream)});
    }

    return true;
}

bool FFMPEGContainer::LoadSubtitleStream(const StreamInfo& streamInfo)
{
    if (!streamInfo.subtitleIndex)
    {
        LogError("FFMPEGContainer::LoadSubtitleStream: Subtitle stream has no subtitle index set");
        return false;
    }

    ActivateSubtitle(SubtitleSource(m_url, *streamInfo.subtitleIndex));

    return true;
}

std::expected<std::unique_ptr<FFMPEGContainer::FFMPEGStream>, bool> FFMPEGContainer::OpenAVStream(
    AVFormatContext *pAVFormatContext,
    unsigned int streamIndex,
    bool supportMultiThreadedDecode,
    bool supportHardwareDecode)
{
    LogInfo("FFMPEGContainer: Opening stream index: {}", (int) streamIndex);

    const auto streamInfoIt = m_streamInfos.find(streamIndex);
    if (streamInfoIt == m_streamInfos.cend())
    {
        LogError("FFMPEGContainer::OpenStream: Unsupported stream index: {}", streamIndex);
        return std::unexpected(false);
    }

    const auto pCodec = avcodec_find_decoder((AVCodecID) streamInfoIt->second.codecID);
    if (!pCodec)
    {
        LogError("FFMPEGContainer::OpenStream: Unable to find decoder: {}", (int) streamInfoIt->second.codecID);
        return std::unexpected(false);
    }

    auto pStream = std::make_unique<FFMPEGStream>();
    pStream->streamIndex = (int) streamIndex;
    pStream->pCodec = pCodec;
    pStream->pStream = pAVFormatContext->streams[pStream->streamIndex];

    //LogInfo("Stream index {} for AV type {} info dump:", pStream->streamIndex, (int)avMediaType);
    //av_dump_format(pAVFormatContext, pStream->streamIndex, nullptr, 0);

    //
    // Allocate a codec context for the video stream
    //
    pStream->pCodecContext = avcodec_alloc_context3(pStream->pCodec);
    if (!pStream->pCodecContext)
    {
        LogError("FFMPEGContainer::OpenStream: avcodec_alloc_context3 failed");
        DestroyStreamObjects(pStream.get());
        return std::unexpected(false);
    }

    //
    // Configure the video codec context based on the values from the stream's video codec parameters
    //
    if (const int result = avcodec_parameters_to_context(pStream->pCodecContext, pStream->pStream->codecpar); result < 0)
    {
        LogError("FFMPEGContainer::OpenStream: avcodec_parameters_to_context failed, error: {}", AVErrorStr(result));
        DestroyStreamObjects(pStream.get());
        return std::unexpected(false);
    }

    //
    // Configure codec multithreading, if available/desired
    //
    if (supportMultiThreadedDecode)
    {
        // Configure for auto-detection of threads
        pStream->pCodecContext->thread_count = 0;

        if (pStream->pCodec->capabilities & AV_CODEC_CAP_FRAME_THREADS)
        {
            LogInfo("FFMPEGContainer::OpenStream: Configuring stream {} codec for FF_THREAD_FRAME multi-threaded decoding", pStream->streamIndex);
            pStream->pCodecContext->thread_type = FF_THREAD_FRAME;
        }
        else if (pStream->pCodec->capabilities & AV_CODEC_CAP_SLICE_THREADS)
        {
            LogInfo("FFMPEGContainer::OpenStream: Configuring stream {} codec for FF_THREAD_SLICE multi-threaded decoding", pStream->streamIndex);
            pStream->pCodecContext->thread_type = FF_THREAD_SLICE;
        }
        else
        {
            LogInfo("FFMPEGContainer::OpenStream: Configuring stream {} codec for single thread decoding",
                    pStream->streamIndex);
            pStream->pCodecContext->thread_count = 1;
        }
    }

    //
    // Configure codec hardware decode, if available/desired
    //
    if (supportHardwareDecode)
    {
        const AVCodecHWConfig *pCodecDeviceHWConfig{nullptr};

        // See if the codec has hardware decode capability
        for (int i = 0;; ++i)
        {
            const auto hwConfig = avcodec_get_hw_config(pStream->pCodec, i);
            if (hwConfig == nullptr)
            {
                LogWarning("FFMPEGContainer::OpenStream: Codec doesn't support hardware device decode");
                break;
            }

            if (hwConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
            {
                pCodecDeviceHWConfig = hwConfig;
                break;
            }
        }

        // If we found hardware decode capability, create a hwdevice context and enable hardware decode
        if (pCodecDeviceHWConfig != nullptr)
        {
            // Store the hw pixel format as heap memory in the stream's data so that it can be set in the
            // stream's user-provided opaque data, so that GetHWFormat can return the desired pixel format
            // for the stream when ffmpeg calls it (c-style pointer limitations)
            pStream->pHWAVPixelFormat = std::make_unique<AVPixelFormat>(pCodecDeviceHWConfig->pix_fmt);
            pStream->pCodecContext->opaque = pStream->pHWAVPixelFormat.get();
            pStream->pCodecContext->get_format = GetHWFormat;

            if (const int ret = av_hwdevice_ctx_create(
                    &pStream->pHWDeviceContext,
                    pCodecDeviceHWConfig->device_type,
                    nullptr,
                    nullptr,
                    0);
                ret < 0)
            {
                LogError("FFMPEGContainer::OpenStream: av_hwdevice_ctx_create failed, error: {}", AVErrorStr(ret));
                DestroyStreamObjects(pStream.get());
                return std::unexpected(false);
            }

            pStream->pCodecContext->hw_device_ctx = av_buffer_ref(pStream->pHWDeviceContext);
        }
    }

    //
    // Opens the codec within the stream's codec context
    //
    if (const int result = avcodec_open2(pStream->pCodecContext, pStream->pCodec, nullptr); result < 0)
    {
        LogError("FFMPEGContainer::OpenStream: avcodec_open2 failed, error: {}", AVErrorStr(result));
        DestroyStreamObjects(pStream.get());
        return std::unexpected(false);
    }

    //
    // Allocate work buffers
    //
    pStream->pFrame = av_frame_alloc();
    if (pStream->pFrame == nullptr)
    {
        LogError("FFMPEGContainer::OpenStream: av_frame_alloc failed");
        DestroyStreamObjects(pStream.get());
        return std::unexpected(false);
    }

    pStream->pHWDestFrame = av_frame_alloc();
    if (pStream->pHWDestFrame == nullptr)
    {
        LogError("FFMPEGContainer::OpenStream: av_frame_alloc failed");
        DestroyStreamObjects(pStream.get());
        return std::unexpected(false);
    }

    pStream->pFiltFrame = av_frame_alloc();
    if (pStream->pFiltFrame == nullptr)
    {
        LogError("FFMPEGContainer::OpenStream: av_frame_alloc failed");
        DestroyStreamObjects(pStream.get());
        return std::unexpected(false);
    }

    return pStream;
}

void FFMPEGContainer::Destroy()
{
    //
    // Destroy stream objects
    //
    for (auto& openStream: m_openStreams)
    {
        DestroyStreamObjects(openStream.second.get());
    }
    m_openStreams.clear();

    //
    // Destroy other ffmpeg resources
    //
    if (m_pPacket != nullptr)
    {
        av_packet_free(&m_pPacket);
    }

    if (m_pFormatContext != nullptr)
    {
        avformat_close_input(&m_pFormatContext);
    }

    //
    // Reset state
    //
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_eof = false;
    m_audioSyncDiff = MediaDuration(0.0);
}

void FFMPEGContainer::DestroyStreamObjects(FFMPEGStream *pStream)
{
    DestroySWRObjects(pStream);

    DestroyFilterGraphObjects(pStream);

    if (pStream->pHWDeviceContext != nullptr)
    {
        av_buffer_unref(&pStream->pHWDeviceContext);
    }

    if (pStream->pCodecContext != nullptr)
    {
        avcodec_free_context(&pStream->pCodecContext);
    }

    if (pStream->pHWDestFrame != nullptr)
    {
        av_frame_free(&pStream->pHWDestFrame);
    }

    if (pStream->pFiltFrame != nullptr)
    {
        av_frame_free(&pStream->pFiltFrame);
    }

    if (pStream->pFrame != nullptr)
    {
        av_frame_free(&pStream->pFrame);
    }

    // Note that these were just references and don't need to be freed
    pStream->pCodec = nullptr;
    pStream->pStream = nullptr;

    pStream->streamIndex = -1;
}

void FFMPEGContainer::DestroySWRObjects(FFMPEGStream *pStream)
{
    pStream->swrConfig = std::nullopt;

    if (pStream->swrContext)
    {
        swr_free(&pStream->swrContext);
    }
}

void FFMPEGContainer::DestroyFilterGraphObjects(FFMPEGStream *pStream)
{
    if (pStream->pAVFilterGraph)
    {
        avfilter_graph_free(&pStream->pAVFilterGraph);
    }

    if (pStream->pAVFilterInputs)
    {
        avfilter_inout_free(&pStream->pAVFilterInputs);
    }

    if (pStream->pAVFilterOutputs)
    {
        avfilter_inout_free(&pStream->pAVFilterOutputs);
    }

    pStream->pAVFilterBufferSrcCtx = nullptr;
    pStream->pAVFilterBufferSinkCtx = nullptr;
}

MediaDuration FFMPEGContainer::GetSourceDuration() const
{
    return MediaDuration((double) m_pFormatContext->duration / (double) AV_TIME_BASE);
}

std::optional<std::pair<unsigned int, unsigned int>> FFMPEGContainer::GetVideoStreamDimensions() const
{
    if (m_videoStreamIndex == -1)
    {
        return std::nullopt;
    }

    const auto& videoStream = m_openStreams.at(m_videoStreamIndex);

    return std::make_pair(videoStream->pCodecContext->width, videoStream->pCodecContext->height);
}

std::expected<AVPacket*, FFMPEGContainer::ReadException> FFMPEGContainer::ReadPacket()
{
    const int result = av_read_frame(m_pFormatContext, m_pPacket);
    if (result < 0)
    {
        // EOF condition
        if (result == AVERROR_EOF)
        {
            m_eof = true;
            return std::unexpected(ReadException::Eof);
        }

        // Error condition
        LogError("FFMPEGContainer::Thread_ReadNextPacket: Error reading packet, error: {}", AVErrorStr(result));
        return std::unexpected(ReadException::Error);
    }

    m_eof = false;

    return m_pPacket;
}

std::optional<FFMPEGContainer::SendPacketException>
FFMPEGContainer::SendPacketToDecoder(MediaStreamType mediaStreamType,
                                     const AVPacket *pPacket)
{
    const auto streamIndex = MediaStreamTypeToStreamIndex(mediaStreamType);
    if (streamIndex == -1)
    {
        LogError("FFMPEGContainer::SendPacketToDecoder: No open stream for media stream type: ", (int) mediaStreamType);
        return SendPacketException::Error;
    }

    const auto stream = m_openStreams.find(streamIndex);
    if (stream == m_openStreams.cend())
    {
        LogError("FFMPEGContainer::SendPacketToDecoder: Can't decode packet for stream which isn't open: {}",
                 streamIndex);
        return SendPacketException::Error;
    }

    const auto result = avcodec_send_packet(stream->second->pCodecContext, pPacket);

    // Successfully sent
    if (result == 0)
    {
        return std::nullopt;
    }

    // Decoder is full and can't take more
    if (result == AVERROR(EAGAIN))
    {
        return SendPacketException::Full;
    }

    // Error condition
    LogError("FFMPEGContainer::SendPacketToDecoder: avcodec_send_packet error: {}", AVErrorStr(result));
    return SendPacketException::Error;
}

std::vector<VideoFrame> FFMPEGContainer::ReceiveVideoFramesFromDecoder()
{
    //
    // Look up the current video stream
    //
    const int streamIndex = MediaStreamTypeToStreamIndex(MediaStreamType::Video);
    if (streamIndex < 0)
    {
        LogError("FFMPEGContainer::ReceiveVideoFramesFromDecoder: Can't determine video stream index");
        return {};
    }

    auto streamIt = m_openStreams.find(streamIndex);
    if (streamIt == m_openStreams.cend())
    {
        LogError("FFMPEGContainer::ReceiveVideoFramesFromDecoder: Can't receive frames for stream which isn't open: {}", streamIndex);
        return {};
    }

    const auto& stream = streamIt->second;

    //
    // Exhaust the decoder of available/decoded frames
    //
    std::vector<AVFrame*> avFrames;

    while (true)
    {
        const auto frameReceiveError = ReceiveFrameFromDecoder(stream.get(), stream->pFrame);
        if (!frameReceiveError)
        {
            // Note: Cloning from the stream working frame to a new frame
            auto pClonedFrame = av_frame_clone(stream->pFrame);
            avFrames.push_back(pClonedFrame);
            av_frame_unref(stream->pFrame);

            // Loop again and receive the next frame, if available
            continue;
        }

        // If the decoder has nothing left to give us, stop
        if (*frameReceiveError == ReceiveFrameException::Dry || *frameReceiveError == ReceiveFrameException::Eof)
        {
            break;
        }

        // Otherwise, any other error is a real error
        LogError("FFMPEGContainer::ReceiveVideoFramesFromDecoder: Error receiving frame from decoder");
        break;
    }

    // If the decoder had no frames ready for us, nothing further to do
    if (avFrames.empty())
    {
        return {};
    }

    //
    // If we're hardware decoding, transfer the decoded frame data from HW to SW
    //
    std::vector<AVFrame*> imageContentFrames;
    std::vector<bool> hwFrameAllocated;

    const auto freeAllocatedMemory = [&](){
        // Frames allocated for hw data transfer
        for (std::size_t x = 0; x < imageContentFrames.size(); ++x)
        {
            if (hwFrameAllocated.at(x))
            {
                av_frame_free(&imageContentFrames.at(x));
            }
        }

        // Frames allocated to hold frame data
        for (auto& avFrame: avFrames)
        {
            av_frame_free(&avFrame);
        }
    };

    for (const auto& avFrame: avFrames)
    {
        if (stream->pHWAVPixelFormat && (avFrame->format == *stream->pHWAVPixelFormat))
        {
            AVFrame *pHWFrame = av_frame_alloc();

            // Transfer hw image data to the stream's HW worker frame
            if (int error = av_hwframe_transfer_data(pHWFrame, avFrame, 0); error < 0)
            {
                LogError("FFMPEGMediaSource::ReceiveVideoFramesFromDecoder: av_hwframe_transfer_data failed, error: {}", AVErrorStr(error));
                freeAllocatedMemory();
                return {};
            }

            pHWFrame->hw_frames_ctx = stream->pHWDeviceContext;

            imageContentFrames.emplace_back(pHWFrame);
            hwFrameAllocated.emplace_back(true);
        }
        else
        {
            imageContentFrames.emplace_back(avFrame);
            hwFrameAllocated.emplace_back(false);
        }
    }

    //
    // Convert the frame image datas to an ImageData
    //
    if (!InsertAVFramesIntoFilterGraph(stream.get(), imageContentFrames))
    {
        LogError("FFMPEGContainer::ReceiveVideoFramesFromDecoder: Error while inserting AVFrames into filter graph");
        freeAllocatedMemory();
        return {};
    }

    const auto frameImages = ReceiveImageDatasFromFilterGraph(stream.get());

    if (frameImages.empty())
    {
        LogError("FFMPEGContainer::ReceiveVideoFramesFromDecoder: No image datas received from filter graph");
        freeAllocatedMemory();
        return {};
    }

    //
    // Convert fetched AVFrames into VideoFrames
    //
    std::vector<VideoFrame> videoFrames;

    for (std::size_t x = 0; x < avFrames.size(); ++x)
    {
        const auto& avFrame = avFrames.at(x);
        const auto& frameImage = frameImages.at(x);

        auto pts = avFrame->best_effort_timestamp;
        if (pts == 0)
        {
            pts = avFrame->pts;
        }
        const auto timeBase = stream->pStream->time_base;
        const auto timeBaseDouble = av_q2d(timeBase);

        videoFrames.push_back(VideoFrame
          {
              .pts            = pts,
              .timeBase       = timeBaseDouble,
              .presentPoint   = MediaPoint((double) pts * timeBaseDouble),
              .imageData      = frameImage
          });
    }

    //
    // Free data allocated for this function's work
    //
    freeAllocatedMemory();

    return videoFrames;
}

std::vector<AudioFrame> FFMPEGContainer::ReceiveAudioFramesFromDecoder()
{
    //
    // Look up the current audio stream
    //
    const int streamIndex = MediaStreamTypeToStreamIndex(MediaStreamType::Audio);
    if (streamIndex == -1)
    {
        LogError("FFMPEGContainer::ReceiveAudioFramesFromDecoder: Can't determine audio stream index");
        return {};
    }

    auto streamIt = m_openStreams.find(streamIndex);
    if (streamIt == m_openStreams.cend())
    {
        LogError("FFMPEGContainer::ReceiveAudioFramesFromDecoder: Can't receive frame for stream which isn't open: {}", streamIndex);
        return {};
    }

    const auto& stream = streamIt->second;

    //
    // Exhaust the decoder of available/decoded frames
    //
    std::vector<AVFrame *> avFrames;

    while (true)
    {
        const auto frameReceiveError = ReceiveFrameFromDecoder(stream.get(), stream->pFrame);
        if (!frameReceiveError)
        {
            // Note: Cloning from the stream working frame to a new frame
            avFrames.push_back(av_frame_clone(stream->pFrame));
            av_frame_unref(stream->pFrame);
            continue;
        }

        // If the decoder has nothing left to give us, stop
        if (*frameReceiveError == ReceiveFrameException::Dry || *frameReceiveError == ReceiveFrameException::Eof)
        {
            break;
        }

        // Otherwise, any other error is a real error
        LogError("FFMPEGContainer::ReceiveAudioFrameFromDecoder: Error receiving frame from decoder");
        break;
    }

    // If the decoder had no frames ready for us, nothing further to do
    if (avFrames.empty())
    {
        return {};
    }

    //
    // Convert the audio frames to AudioDatas
    //
    std::vector<Common::AudioData::Ptr> audioDatas;

    for (const auto& avFrame: avFrames)
    {
        const auto audioData = ConvertAVFrameToAudio(stream.get(), avFrame);
        if (audioData)
        {
            audioDatas.push_back(*audioData);
        }
        else
        {
            LogError("FFMPEGContainer::ReceiveAudioFrameFromDecoder: Failed to convert an AVFrame to audio");
            audioDatas.push_back(nullptr);
        }
    }

    //
    // Put together AudioFrame collection to be returned
    //
    std::vector<AudioFrame> audioFrames;

    for (std::size_t x = 0; x < avFrames.size(); ++x)
    {
        const auto& avFrame = avFrames.at(x);
        const auto& audioData = audioDatas.at(x);

        if (audioData == nullptr)
        { continue; }

        auto pts = avFrame->best_effort_timestamp;
        if (pts == 0)
        {
            pts = avFrame->pts;
        }

        const auto timeBase = stream->pStream->time_base;
        const auto timeBaseDouble = av_q2d(timeBase);

        audioFrames.push_back(AudioFrame
          {
              .pts            = pts,
              .timeBase       = timeBaseDouble,
              .presentPoint   = MediaPoint((double) pts * timeBaseDouble),
              .audioData      = audioData
          });
    }

    //
    // Free data allocated for this function's work
    //

    // Frames allocated to hold frame data
    for (auto& avFrame : avFrames)
    {
        av_frame_free(&avFrame);
    }

    return audioFrames;
}

std::optional<SubtitleFrame> FFMPEGContainer::DecodeSubtitle(const AVPacket *pPacket)
{
    assert(false);
    (void)pPacket;
    return std::nullopt;

    /*if (m_subtitleStreamIndex == -1)
    {
        LogError("FFMPEGContainer::DecodeSubtitle: No subtitle stream is active");
        return std::nullopt;
    }

    auto streamIt = m_openStreams.find(m_subtitleStreamIndex);
    if (streamIt == m_openStreams.cend())
    {
        LogError("FFMPEGContainer::DecodeSubtitle: Can't receive frame for stream which isn't open: {}", m_subtitleStreamIndex);
        return std::nullopt;
    }

    const auto& stream = streamIt->second;

    AVSubtitle avSubtitle{};
    int gotSubtitle = 0;

    const auto ret = avcodec_decode_subtitle2(stream->pCodecContext, &avSubtitle, &gotSubtitle, pPacket);
    if (ret < 0)
    {
        LogError("FFMPEGContainer::DecodeSubtitle: Error decoding subtitle, error: {}", AVErrorStr(ret));
        return std::nullopt;
    }

    if (gotSubtitle && (avSubtitle.format != 0))
    {
        if (avSubtitle.rects[0]->ass != nullptr)
        {
            LogError("Subttitle: {}", avSubtitle.rects[0]->ass);
        }

        avsubtitle_free(&avSubtitle);
        //return std::nullopt; // TODO!
    }*/

    /*if (!gotSubtitle)
    {
        LogError("FFMPEGContainer::DecodeSubtitle: No subtitle returned");
        return std::nullopt;
    }*/

    //return SubtitleFrame{};
}

int FFMPEGContainer::MediaStreamTypeToStreamIndex(MediaStreamType mediaStreamType) const
{
    int streamIndex = -1;

    switch (mediaStreamType)
    {
        case MediaStreamType::Video: streamIndex = m_videoStreamIndex; break;
        case MediaStreamType::Audio: streamIndex = m_audioStreamIndex; break;
        default: streamIndex = -1; break;
    }

    return streamIndex;
}

void FFMPEGContainer::ActivateSubtitle(const SubtitleSource& subtitleSource)
{
    LogInfo("FFMPEGContainer: Activating subtitle: {} , subtitle index: {}", subtitleSource.url, subtitleSource.subtitleIndex);

    // Update the subtitle source, which will cause the filter graph to be recreated the next time a video frame
    // is displayed, since the subtitle source is part of the filter graph's config
    m_subtitleSource = subtitleSource;
}

std::optional<FFMPEGContainer::ReceiveFrameException> FFMPEGContainer::ReceiveFrameFromDecoder(FFMPEGStream* pStream, AVFrame* pOutput)
{
    //
    // Try to fetch a frame from the stream's decoder
    //
    const auto result = avcodec_receive_frame(pStream->pCodecContext, pOutput);

    // Successfully received
    if (result == 0)
    {
        return std::nullopt;
    }
    // Decoder needs more data sent
    else if (result == AVERROR(EAGAIN))
    {
        return ReceiveFrameException::Dry;
    }
    else if (result == AVERROR_EOF)
    {
        return ReceiveFrameException::Eof;
    }

    // Error condition
    LogError("FFMPEGContainer::ReceiveFrameFromDecoder: avcodec_receive_frame error: {}", AVErrorStr(result));
    return ReceiveFrameException::Error;
}

bool FFMPEGContainer::InsertAVFramesIntoFilterGraph(FFMPEGStream* pStream, const std::vector<AVFrame*>& frames)
{
    const AVPixelFormat convertToFormat = AV_PIX_FMT_RGBA;

    if (!CreateVideoFilterGraphAsNeeded(pStream, convertToFormat, m_subtitleSource))
    {
        LogError("FFMPEGContainer::InsertAVFramesIntoFilterGraph: Failed to create filter graph");
        return false;
    }

    for (const auto& pAVFrame : frames)
    {
        if (const auto ret = av_buffersrc_add_frame_flags(pStream->pAVFilterBufferSrcCtx, pAVFrame, AV_BUFFERSRC_FLAG_KEEP_REF); ret < 0)
        {
            LogError("FFMPEGContainer::InsertAVFramesIntoFilterGraph: av_buffersrc_add_frame_flags failed, error: {}", AVErrorStr(ret));
        }
    }

    return true;
}

std::vector<Common::ImageData::Ptr> FFMPEGContainer::ReceiveImageDatasFromFilterGraph(FFMPEGStream* pStream)
{
    std::vector<Common::ImageData::Ptr> imageDatas;

    while (true)
    {
        const auto ret = av_buffersink_get_frame(pStream->pAVFilterBufferSinkCtx, pStream->pFiltFrame);

        if (ret == AVERROR(EAGAIN))
        {
            return imageDatas;
        }
        else if (ret < 0)
        {
            LogError("FFMPEGContainer::ReceiveImageDatasFromFilterGraph: av_buffersink_get_frame error: {}", AVErrorStr(ret));
            return imageDatas;
        }

        const auto imageWidth = pStream->pFiltFrame->width;
        const auto imageHeight = pStream->pFiltFrame->height;
        const auto bytesPerPixel = 4;

        std::vector<std::byte> imageBytes(imageWidth * imageHeight * bytesPerPixel);

        for (int y = 0; y < std::abs(pStream->pFiltFrame->height); ++y)
        {
            memcpy(imageBytes.data() + (y * pStream->pFiltFrame->width * bytesPerPixel),
                   pStream->pFiltFrame->data[0] + (y * pStream->pFiltFrame->linesize[0]),
                   pStream->pFiltFrame->width * bytesPerPixel);
        }

        av_frame_unref(pStream->pFiltFrame);

        imageDatas.push_back(std::make_shared<Common::ImageData>(
            imageBytes,
            1,
            imageWidth,
            imageHeight,
            Common::ImageData::PixelFormat::RGBA32
        ));
    };
}

// Returns filters string for buffer filter
std::string FilterGraphConfigToBufferFilterArgs(const FilterGraphConfig& filterGraphConfig)
{
    return std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
        filterGraphConfig.srcWidth,
        filterGraphConfig.srcHeight,
        (int)filterGraphConfig.srcPixelFormat,
        filterGraphConfig.srcTimeBase.num,
        filterGraphConfig.srcTimeBase.den,
        filterGraphConfig.srcAspectRatio.num,
        filterGraphConfig.srcAspectRatio.den
    );
}

// TODO!
//  Works from prompt: ffplay -vf subtitles=filename="aa \\\\\\'bb\\\\\\' cc.mkv" "aa 'bb' cc.mkv"

std::string EscapeStringForFilter(const std::string& string)
{
    char* pEscapedString{nullptr};

    const auto ret = av_escape(&pEscapedString, string.c_str(), nullptr, AV_ESCAPE_MODE_BACKSLASH, 0);
    if (ret < 0)
    {
        return string;
    }

    std::string escapedString(pEscapedString);

    av_free(pEscapedString);

    return escapedString;
}

// Returns top-level filter graph string
std::string FilterGraphConfigToGraphFilters(const FilterGraphConfig& filterGraphConfig)
{
    std::string filters;

    if (filterGraphConfig.subtitleSource)
    {
        filters = std::format("subtitles=filename='{}':si={}",
          EscapeStringForFilter(EscapeStringForFilter(filterGraphConfig.subtitleSource->url)),
          filterGraphConfig.subtitleSource->subtitleIndex
        );
    }

    if (filters.empty())
    {
        filters = "null";
    }

    return filters;
}

FilterGraphConfig FFMPEGContainer::GetFilterGraphConfig(FFMPEGStream* pVideoStream,
                                                        AVPixelFormat destPixelFormat,
                                                        const std::optional<SubtitleSource>& subtitleSource) const
{
    return {
        pVideoStream->pCodecContext->width,
        pVideoStream->pCodecContext->height,
        pVideoStream->pCodecContext->pix_fmt,
        pVideoStream->pStream->time_base,
        pVideoStream->pStream->sample_aspect_ratio,
        destPixelFormat,
        subtitleSource
    };
}

bool FFMPEGContainer::CreateVideoFilterGraphAsNeeded(FFMPEGStream* pVideoStream,
                                                     AVPixelFormat destPixelFormat,
                                                     const std::optional<SubtitleSource>& subtitleSource)
{
    const auto filterGraphConfig = GetFilterGraphConfig(pVideoStream, destPixelFormat, subtitleSource);

    // If a filter graph with a matching config already exists, re-use it
    if ((pVideoStream->pAVFilterGraph != nullptr) &&
        pVideoStream->filterGraphConfig &&
        (*pVideoStream->filterGraphConfig == filterGraphConfig))
    {
        return true;
    }

    // If the filter graph config is the same as the previous one, and we failed to create a filter
    // graph from the previous one, don't attempt to create a new one and have it fail again
    if ((pVideoStream->pAVFilterGraph == nullptr) &&
        pVideoStream->filterGraphConfig &&
        (*pVideoStream->filterGraphConfig == filterGraphConfig))
    {
        return false;
    }

    // Otherwise, destroy any existing filter graph objects, to be recreated below
    DestroyFilterGraphObjects(pVideoStream);

    //
    // Create a filter graph
    //
    LogInfo("FFMPEGContainer: Creating new filter graph");

    // Explicitly record the config before doing any work below that might fail, so that we have
    // a record of the config that it failed with
    pVideoStream->filterGraphConfig = filterGraphConfig;

    // Buffer filter
    const AVFilter* pBufferSrc  = avfilter_get_by_name("buffer");
    if (pBufferSrc == nullptr)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: No such avfilter exists: buffer");
        return false;
    }

    // Buffersink filter
    const AVFilter* pBufferSink = avfilter_get_by_name("buffersink");
    if (pBufferSink == nullptr)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: No such avfilter exists: buffersink");
        return false;
    }

    pVideoStream->pAVFilterInputs = avfilter_inout_alloc();
    if (pVideoStream->pAVFilterInputs == nullptr)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: avfilter_inout_alloc failed");
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    pVideoStream->pAVFilterOutputs = avfilter_inout_alloc();
    if (pVideoStream->pAVFilterOutputs == nullptr)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: avfilter_inout_alloc failed");
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    pVideoStream->pAVFilterGraph = avfilter_graph_alloc();
    if (pVideoStream->pAVFilterGraph == nullptr)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: avfilter_graph_alloc failed");
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    // Configure the filter graph
    pVideoStream->pAVFilterGraph->nb_threads = 0; // automatic thread determination.

    //
    // Create and add filter graph filters
    //

    // Buffer source filter
    const auto bufferFilterArgs = FilterGraphConfigToBufferFilterArgs(filterGraphConfig);

    if (const auto ret = avfilter_graph_create_filter(
        &pVideoStream->pAVFilterBufferSrcCtx,
        pBufferSrc,
        "in",
        bufferFilterArgs.c_str(),
        nullptr,
        pVideoStream->pAVFilterGraph); ret < 0)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: avfilter_graph_create_filter failed, error: {}", AVErrorStr(ret));
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    // Buffer sink filter
    if (const auto ret = avfilter_graph_create_filter(
        &pVideoStream->pAVFilterBufferSinkCtx,
        pBufferSink,
        "out",
        nullptr,
        nullptr,
        pVideoStream->pAVFilterGraph); ret < 0)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: avfilter_graph_create_filter failed, error: {}", AVErrorStr(ret));
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    enum AVPixelFormat destPixFmts[] = { destPixelFormat, AV_PIX_FMT_NONE };

    if (const auto ret = av_opt_set_int_list(
        pVideoStream->pAVFilterBufferSinkCtx,
        "pix_fmts",
        destPixFmts,
        AV_PIX_FMT_NONE,
        AV_OPT_SEARCH_CHILDREN); ret < 0)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: av_opt_set_int_list failed, error: {}", AVErrorStr(ret));
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    pVideoStream->pAVFilterOutputs->name       = av_strdup("in");
    pVideoStream->pAVFilterOutputs->filter_ctx = pVideoStream->pAVFilterBufferSrcCtx;
    pVideoStream->pAVFilterOutputs->pad_idx    = 0;
    pVideoStream->pAVFilterOutputs->next       = nullptr;

    pVideoStream->pAVFilterInputs->name       = av_strdup("out");
    pVideoStream->pAVFilterInputs->filter_ctx = pVideoStream->pAVFilterBufferSinkCtx;
    pVideoStream->pAVFilterInputs->pad_idx    = 0;
    pVideoStream->pAVFilterInputs->next       = nullptr;

    const std::string graphFilters = FilterGraphConfigToGraphFilters(filterGraphConfig);

    if (const auto ret = avfilter_graph_parse_ptr(
        pVideoStream->pAVFilterGraph,
        graphFilters.c_str(),
        &pVideoStream->pAVFilterInputs,
        &pVideoStream->pAVFilterOutputs,
        nullptr); ret < 0)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: avfilter_graph_parse_ptr failed, error: {}", AVErrorStr(ret));
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    if (const auto ret = avfilter_graph_config(pVideoStream->pAVFilterGraph, nullptr); ret < 0)
    {
        LogError("FFMPEGContainer::CreateVideoFilterGraphAsNeeded: avfilter_graph_config failed, error: {}", AVErrorStr(ret));
        DestroyFilterGraphObjects(pVideoStream);
        return false;
    }

    return true;
}

bool FFMPEGContainer::CreateAudioSwrAsNeeded(FFMPEGStream* pStream,
                                             const AVChannelLayout& destChannelLayout,
                                             AVSampleFormat destSampleFormat,
                                             int destSampleRate)
{
    const auto swrConfig = SWRConfig(
        pStream->pCodecContext->ch_layout,
        pStream->pCodecContext->sample_fmt,
        pStream->pCodecContext->sample_rate,
        destChannelLayout,
        destSampleFormat,
        destSampleRate
    );

    // If the existing config matches and an swr context already exists, nothing to do
    if (pStream->swrConfig && pStream->swrContext)
    {
        if (*pStream->swrConfig == swrConfig)
        {
            return true;
        }
    }

    // Otherwise, destroy any existing swr context, create a new one below
    DestroySWRObjects(pStream);

    //
    // Create and configure an SWR context
    //
    LogInfo("FFMPEGContainer: Creating new SWR context");

    pStream->swrContext = swr_alloc();

    if (const auto ret = swr_alloc_set_opts2(
        &pStream->swrContext,
        &swrConfig.destChannelLayout,
        swrConfig.destSampleFormat,
        swrConfig.destSampleRate,
        &swrConfig.srcChannelLayout,
        swrConfig.srcSampleFormat,
        swrConfig.srcSampleRate,
        0,
        nullptr); ret < 0)
    {
        LogError("FFMPEGContainer::GetSwrContext: swr_alloc_set_opts2 failed, error: {}", AVErrorStr(ret));
        DestroySWRObjects(pStream);
        return false;
    }

    if (const auto ret = swr_init(pStream->swrContext); ret < 0)
    {
        LogError("FFMPEGContainer::GetSwrContext: swr_init failed, error: {}", AVErrorStr(ret));
        DestroySWRObjects(pStream);
        return false;
    }

    pStream->swrConfig = swrConfig;

    return true;
}

int FFMPEGContainer::GetSyncAdjustedNumSamples(int frameNumSamples, int frameSampleRate)
{
    // Returns an adjusted, desired, number of samples a frame should last for, in order to
    // reduce the current audio sync offset. (If the audio is running slow, reduce samples to catch up,
    // and vice versa). Tries to reduce the audio sync offset over a number of frames rather than all at
    // once (by maintaining the running m_audioSyncDiff value), as well as limiting how much a single
    // frame's sample count can be manipulated to avoid noticeable audio changes, and reducing the amount
    // of manipulation that's allowed, the closer the audio sync gets to the minimum allowed sync diff.

    // Make a copy of the current audio sync diff, as it could be updated in parallel as the media session
    // recalculates the current audio sync
    const auto audioSyncDiff = m_audioSyncDiff;

    // If the audio is out of sync by less than a minimum amount, don't try to adjust for it
    if (std::abs(audioSyncDiff.count()) < MIN_SYNC_ADJUSTMENT_LEVEL.count())
    {
        return frameNumSamples;
    }

    // How many samples out of sync the audio stream is
    const auto numSampleSyncDiff = audioSyncDiff.count() / (1.0 / (double)frameSampleRate);

    // Calculate the maximum percentage by which we'll allow tweaking the number of samples. The higher
    // the audio offset, the higher the percentage we'll allow, up until a cap. Linear func.
    const float yAxisMax = 0.04;    // 4.0% - Maximum allowed percentage change
    const float xAxisMax = 0.2;     // 200ms - The sync diff at which maximum percentage change is allowed
    const float slope = yAxisMax / xAxisMax;

    const auto absAudioSyncDiff = std::abs(audioSyncDiff.count());
    const auto maxAdjustPercentage = std::clamp(slope * absAudioSyncDiff, 0.0f, yAxisMax);
    const auto maxSampleAdjustment = (double)frameNumSamples * maxAdjustPercentage;

    // The number of samples to adjust the playback by. Clamp the number of samples the stream is off by to
    // the maximum allowed sample adjustment.
    const auto numSamplesAdjustOffset = std::clamp(numSampleSyncDiff, -maxSampleAdjustment, maxSampleAdjustment);

    // The amount of time (seconds) that numSamplesAdjustOffset number of samples uses. This is the amount of
    // time we were able to correct the sync offset by.
    const auto appliedSyncDiff = numSamplesAdjustOffset * (1.0 / (double)frameSampleRate);
    m_audioSyncDiff -= MediaDuration(appliedSyncDiff);

    return (int)(frameNumSamples + numSamplesAdjustOffset);
}

AVSampleFormat AudioDataFormatToAVSampleFormat(Common::AudioDataFormat audioDataFormat)
{
    switch (audioDataFormat)
    {
        case Common::AudioDataFormat::Mono8:
        case Common::AudioDataFormat::Stereo8:
            return AV_SAMPLE_FMT_U8;
        case Common::AudioDataFormat::Mono16:
        case Common::AudioDataFormat::Stereo16:
            return AV_SAMPLE_FMT_S16;
    }

    assert(false);
    return AV_SAMPLE_FMT_NONE;
}

std::expected<Common::AudioData::Ptr, bool> FFMPEGContainer::ConvertAVFrameToAudio(FFMPEGStream* pStream, AVFrame* pFrame)
{
    const auto sourceNumSamples = pFrame->nb_samples;
    const auto sourceSampleRate = pStream->pCodecContext->sample_rate;

    //
    // Destination/resampled audio data properties
    //
    AVChannelLayout destChannelLayout{};
    av_channel_layout_default(&destChannelLayout, (int)Common::GetAudioFormatNumChannels(m_config.audioOutputFormat));
    AVSampleFormat destSampleFormat = AudioDataFormatToAVSampleFormat(m_config.audioOutputFormat);
    const auto destSampleRate = sourceSampleRate;

    // Get or create an SWR context for resampling the audio from stream format to our desired Stereo16 format. Will
    // re-use a cached SWR context if available, and will re-create the SWR context if any resampling parameters change.
    if (!CreateAudioSwrAsNeeded(pStream, destChannelLayout, destSampleFormat, destSampleRate))
    {
        LogError("FFMPEGContainer::Thread_ConvertAVFrameToAudio: Failed to get/create swr context");
        return std::unexpected(false);
    }

    const auto wantedNumSamples = GetSyncAdjustedNumSamples(sourceNumSamples, sourceSampleRate);

    if (wantedNumSamples != sourceNumSamples)
    {
        if (const auto ret = swr_set_compensation(pStream->swrContext, wantedNumSamples - sourceNumSamples, wantedNumSamples); ret < 0)
        {
            LogError("FFMPEGContainer::Thread_ConvertAVFrameToAudio: swr_set_compensation failed, error: {}", AVErrorStr(ret));
            return std::unexpected(false);
        }
    }

    //
    // Buffer to receive resampled audio data
    //
    uint8_t** destData{nullptr};
    int destLineSize{0}; // Gets populated by av_samples_alloc_array_and_samples

    auto result = av_samples_alloc_array_and_samples(
        &destData,
        &destLineSize,
        destChannelLayout.nb_channels,
        wantedNumSamples,
        destSampleFormat,
        0
    );
    if (result < 0)
    {
        LogError("FFMPEGContainer::Thread_ConvertAVFrameToAudio: av_samples_alloc_array_and_samples failed, error: {}", AVErrorStr(result));
        return std::unexpected(false);
    }

    const auto freeDestData = [&](){
        av_freep(&destData[0]);
        av_freep(&destData);
    };

    result = swr_convert(pStream->swrContext, destData, wantedNumSamples, pFrame->data, sourceNumSamples);
    if (result < 0)
    {
        LogError("FFMPEGContainer::Thread_ConvertAVFrameToAudio: swr_convert failed, error: {}", AVErrorStr(result));
        freeDestData();
        return std::unexpected(false);
    }

    const auto dstBufferSize = av_samples_get_buffer_size(
        &destLineSize,
        destChannelLayout.nb_channels,
        result,
        destSampleFormat,
        1
    );
    if (dstBufferSize < 0)
    {
        LogError("FFMPEGContainer::Thread_ConvertAVFrameToAudio: av_samples_get_buffer_size failed, error: {}", AVErrorStr(result));
        freeDestData();
        return std::unexpected(false);
    }

    std::vector<std::byte> audioBytes;
    audioBytes.resize(dstBufferSize);

    memcpy(audioBytes.data(), destData[0], dstBufferSize);

    freeDestData();

    return std::make_shared<Common::AudioData>(m_config.audioOutputFormat, destSampleRate, std::move(audioBytes));
}

bool FFMPEGContainer::SeekToPoint(const MediaPoint& point, const std::optional<MediaDuration>& relative)
{
    LogInfo("FFMPEGContainer: Seeking to point: {}", point);

    MediaPoint seekPoint = point;

    // Bounds the seek to the duration of the source
    if (seekPoint < MediaPoint(0.0)) { seekPoint = MediaPoint(0.0); }
    if (seekPoint >= GetSourceDuration()) { seekPoint = GetSourceDuration() - MediaDuration(0.1); }

    // Calculate our relative seek offset, if any. Note that we correct for if we had to bounds check
    // seekPoint above; the relative seek will have changed if we had to change the seek point
    std::optional<MediaDuration> seekRelative = relative;
    if (seekRelative)
    {
        *seekRelative += (seekPoint - point);
    }

    //
    // Flush decoders of data
    //
    if (m_videoStreamIndex != -1) { FlushDecoder(MediaStreamType::Video); }
    if (m_audioStreamIndex != -1) { FlushDecoder(MediaStreamType::Audio); }

    //
    // Seek the container to the new point
    //
    const auto targetPTS = (int64_t)(seekPoint.count() * (double)AV_TIME_BASE);
    const auto startingPTS = seekRelative ? (int64_t)((seekPoint - *seekRelative).count() * (double)AV_TIME_BASE) : (int64_t)0;

    auto minTargetPTS = INT64_MIN;
    auto maxTargetPTS = INT64_MAX;

    // If we're seeking forwards, the minimum should be the point we're starting at.
    if (seekRelative && (seekRelative->count() > 0.0)) { minTargetPTS = startingPTS; }

    // If we're seeking backwards, the maximum should be the point we're starting at.
    if (seekRelative && (seekRelative->count() < 0.0)) { maxTargetPTS = startingPTS; }

    if (const auto ret = avformat_seek_file(m_pFormatContext, -1, minTargetPTS, targetPTS, maxTargetPTS, 0); ret < 0)
    {
        LogError("FFMPEGContainer::SeekToPoint: avformat_seek_file failed, error: {}", AVErrorStr(ret));
        return false;
    }

    // Presumably, if we successfully seeked to somewhere in the file, we're not at eof
    m_eof = false;

    return true;
}

void FFMPEGContainer::FlushDecoder(MediaStreamType mediaStreamType)
{
    const auto streamIndex = MediaStreamTypeToStreamIndex(mediaStreamType);
    if (streamIndex == -1)
    {
        LogError("FFMPEGContainer::FlushDecoder: No open stream for media stream type: ", (int)mediaStreamType);
        return;
    }

    const auto stream = m_openStreams.find(streamIndex);
    if (stream == m_openStreams.cend())
    {
        LogError("FFMPEGContainer::FlushDecoder: Can't find stream object for stream index: {}", streamIndex);
        return;
    }

    LogDebug("FFMPEGContainer: Flushing decoder for media stream type: {}", (int)mediaStreamType);

    switch (mediaStreamType)
    {
        case MediaStreamType::Video:
        case MediaStreamType::Audio:
        {
            avcodec_flush_buffers(stream->second->pCodecContext);
        }
        break;
        case MediaStreamType::Subtitle:
        {
            // TODO!
            //avcodec_decode_subtitle2(stream->second->pCodecContext, nullptr, nullptr, nullptr);
        }
        break;
    }
}

}
