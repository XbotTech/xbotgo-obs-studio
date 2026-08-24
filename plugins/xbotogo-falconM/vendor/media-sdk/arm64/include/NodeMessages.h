#pragma once
#include "DataTypes.h"
#include "NodeTypes.h"
#include "GenericObject.h"
#include <string>
#include <map>

namespace blink {
namespace media {

// SrtPullNode
struct NewSrtStreamMessage : public GenericData {
    int32_t mediaType;
    int32_t mediaFormat;
    uint32_t ssrc;
    NewSrtStreamMessage() : ssrc(0), 
        mediaType(MEDIA_DATA_TYPE_UNKNOWN), 
        mediaFormat(MEDIA_DATA_FORMAT_UNKNOWN) {}
};

// SrtPullNode
struct DeleteSrtStreamMessage : public GenericData {
    int32_t mediaType;
    int32_t mediaFormat;
    uint32_t ssrc;
    DeleteSrtStreamMessage() : ssrc(0), 
        mediaType(MEDIA_DATA_TYPE_UNKNOWN), 
        mediaFormat(MEDIA_DATA_FORMAT_UNKNOWN) {}
};

// SrtPullNode
struct SrtPullStatesMessage : public GenericData {
    std::map<uint32_t, int64_t> outputSsrcBytes;
    int64_t timestampMs;
    double rttMs;
    double bandwidthMbps;
    double packetLossRate;
    double packetFilterLossRate;
    int32_t networkQualityLevel;  // Network quality level based on bandwidth, 0=lowest, higher value=better quality
    SrtPullStatesMessage() : timestampMs(0),
        rttMs(0), bandwidthMbps(0), packetLossRate(0), packetFilterLossRate(0), networkQualityLevel(0) {}
};

// PullStream
struct AddSrtDecodeMessage : public GenericData {
    uint32_t ssrc;
    int32_t nodeId;
    AddSrtDecodeMessage() : ssrc(0), nodeId(0) {}
};

// PullStream
struct RemoveSrtDecodeMessage : public GenericData {
    uint32_t ssrc;
    RemoveSrtDecodeMessage() : ssrc(0) {}
};

enum SrtStreamStatusType {
    SRT_STREAM_STATUS_UNKNOWN = -1,
    SRT_STREAM_STATUS_DISCONNECTED = 0,
    SRT_STREAM_STATUS_CONNECTING,
    SRT_STREAM_STATUS_CONNECTED,
    SRT_STREAM_STATUS_MAX
};

enum SrtDisconnectReason {
    SRT_DISCONNECT_REASON_UNKNOWN = -1,
    SRT_DISCONNECT_REASON_CONNECT_FAILED = 0,
    SRT_DISCONNECT_REASON_WRITE_FAILED,
    SRT_DISCONNECT_REASON_STOP,
    SRT_DISCONNECT_REASON_MAX
};

// SrtPushNode
struct SrtStreamStatusMessage : public GenericData {
    int32_t status;
    int32_t reason;
    SrtStreamStatusMessage() : status(SRT_STREAM_STATUS_UNKNOWN), reason(0) {}
};

struct SrtPushStatesMessage : public GenericData {
    std::map<uint32_t, int64_t> inputSsrcBytes;
    std::map<uint32_t, int64_t> outputSsrcBytes;
    int64_t videoInputFrames;
    int64_t totalSendBytes;
    int64_t timestampMs;
    double rttMs;
    double bandwidthMbps;
    double packetLossRate;
    double packetFilterLossRate;
    SrtPushStatesMessage() : videoInputFrames(0), totalSendBytes(0), timestampMs(0),
        rttMs(0), bandwidthMbps(0), packetLossRate(0), packetFilterLossRate(0) {}
};

// VideoDecoderNode
struct VideoFormatChangedMessage : public GenericData {
    int32_t width;
    int32_t height;
    VideoFormatChangedMessage() : width(0), height(0) {}
};

enum VideoDecodeErrorReason {
    VIDEO_DECODE_ERROR_UNKNOWN = -1,
    // Failed to create/configure a decoder (no hw/sw codec found, or configure() rejected the stream).
    VIDEO_DECODE_ERROR_CODEC_UNAVAILABLE = 0,
    // MediaCodec hit an unrecoverable CodecException while decoding.
    VIDEO_DECODE_ERROR_CODEC_EXCEPTION = 1,
    VIDEO_DECODE_ERROR_MAX
};

// VideoDecoderNode; sent only once decoding has been given up on. Recoverable failures are
// retried internally (decoder restart) without notifying upward.
struct VideoDecodeErrorMessage : public GenericData {
    int32_t codecFormat;    // MEDIA_DATA_FORMAT_H264 or MEDIA_DATA_FORMAT_H265
    int32_t reason;         // VideoDecodeErrorReason
    std::string errorMessage;
    VideoDecodeErrorMessage() : codecFormat(MEDIA_DATA_FORMAT_UNKNOWN), reason(VIDEO_DECODE_ERROR_UNKNOWN) {}
};

// VideoDecoderNode
struct VideoDecoderStatesMessage : public GenericData {
    int64_t inputBytes;
    int32_t outputFrames;
    int64_t timestampMs;
    int64_t videoPts;           // Current video PTS (presentation timestamp)
    int64_t latencyMs;          // Latency in milliseconds
    VideoDecoderStatesMessage() : inputBytes(0), outputFrames(0), timestampMs(0), videoPts(0), latencyMs(0) {}
};

// AudioDecoderNode
struct AudioDecoderStatesMessage : public GenericData {
    int64_t inputBytes;         // Total input bytes to decoder
    int64_t outputSamples;      // Total output audio samples
    int64_t timestampMs;        // Timestamp of this statistics
    int64_t audioPts;           // Current audio PTS (presentation timestamp)
    AudioDecoderStatesMessage() : inputBytes(0), outputSamples(0), timestampMs(0), audioPts(0) {}
};

// VideoEncoderNode
struct VideoEncoderStatesMessage : public GenericData {
    int32_t inputFrames;
    int64_t outputBytes;
    int64_t timestampMs;
    int64_t latencyMs;          // Latency in milliseconds
    VideoEncoderStatesMessage() : inputFrames(0), outputBytes(0), timestampMs(0), latencyMs(0) {}
};

// VideoScalerNode
struct VideoScalerSizeChangeMessage : public GenericData {
    int32_t width;
    int32_t height;
    VideoScalerSizeChangeMessage() : width(0), height(0) {}
};

// VideoEncoderNode
struct VideoEncoderConfigChangeMessage : public GenericData {
    int32_t encodeType;     // MEDIA_DATA_FORMAT_H264 or MEDIA_DATA_FORMAT_H265
    int32_t width;
    int32_t height;
    int32_t bitrate;
    int32_t fps;
    int32_t profile;
    int32_t level;
    int32_t format;         // input format
    int32_t keyFrameInterval;

    VideoEncoderConfigChangeMessage() :
        encodeType(-1), width(-1), height(-1), bitrate(-1),
        fps(-1), profile(-1), level(-1), format(-1), keyFrameInterval(-1) {}
};

// VideoFpsNode
struct VideoFpsChangeMessage : public GenericData {
    int32_t fps;
    VideoFpsChangeMessage() : fps(30) {}
};

// DumpMediaNode
struct DumpFileNameChangeMessage : public GenericData {
    std::string fileName;
    DumpFileNameChangeMessage() : fileName("") {}
};

// SrtPushNode
struct SrtPushSetTargetBitrateMessage : public GenericData {
    int32_t targetBitrate;  // Target bitrate in bits per second
    int32_t fecPercentNum;
    int32_t fecPercentDen;
    SrtPushSetTargetBitrateMessage() : targetBitrate(0), fecPercentNum(0), fecPercentDen(0) {}
};

// ExternalMediaNodeAdapterNode (Android), sets/replaces the wrapped Java ExternalMediaNode instance
struct ExternalMediaNodeAdapterSetNodeMessage : public GenericData {
    std::shared_ptr<utils::GenericObject> externalMediaNode;
};

}
}
