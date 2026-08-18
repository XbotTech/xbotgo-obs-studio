#pragma once
#include "DataTypes.h"
#include <cstdint>
#include <string>
#include <set>
#include <functional>
#include "GenericObject.h"

namespace blink {
namespace media {

// Forward declaration for MediaDataObserverCallback
typedef std::function<void(std::shared_ptr<MediaData>)> MediaDataObserverCallback;

// for VideoEncoderXXXX node
struct VideoEncoderNodeConfig: public GenericData {
  int encodeType;
  int width;
  int height;
  int bitrate;
  int fps;
  int profile;
  int level;
  int format;
  int keyFrameInterval;
  bool blockMode;  // If true, block in onFrame when no input buffer available or max encoding tasks exceeded

  VideoEncoderNodeConfig() : encodeType(MEDIA_DATA_FORMAT_H264),
    width(1280), height(720), bitrate(2000000),
    fps(30), profile(-1), level(-1), format(MEDIA_DATA_FORMAT_YUV420P),
    keyFrameInterval(30), blockMode(false) {}
};

// for AudioEncoderXXXX node
struct AudioEncoderNodeConfig : public GenericData {
  int encodeFormat;
  int bitrate;
  int channels;
  int sampleRate;
  int inputFormat;

  AudioEncoderNodeConfig() : encodeFormat(MEDIA_DATA_FORMAT_AAC),
    bitrate(128000), channels(2), sampleRate(44100), inputFormat(MEDIA_DATA_FORMAT_PCM_S16) {}
};

// for FileDump node
struct FileDumpNodeConfig : public GenericData {
    std::string fileName;
    FileDumpNodeConfig() : fileName("") {}
};

enum SrtPacketLossResilienceLevel {
    SRT_PACKET_LOSS_RESILIENCE_LEVEL_NONE = 0,
    SRT_PACKET_LOSS_RESILIENCE_LEVEL_1,
    SRT_PACKET_LOSS_RESILIENCE_LEVEL_2,
    SRT_PACKET_LOSS_RESILIENCE_LEVEL_3,
    SRT_PACKET_LOSS_RESILIENCE_LEVEL_NUMBERS
};

// for SRT Push node
struct SrtPushNodeConfig : public GenericData {
    std::string srtLocalBindIp;
    std::string srtServerIp;
    uint16_t srtServerPort;
    uint32_t videoSsrc;
    uint32_t audioSsrc;
    uint32_t dataSsrc; // for object detection result ssrc

    int64_t targetBitrate; // for paced sender to control bitrate
    SrtPacketLossResilienceLevel packetLossResilienceLevel;
    int32_t peerLatencyMs; // in ms, for SRT socket option SRTO_PEERLATENCY, -1 for no change
    int32_t fecPercentNum; // for SRT socket option SRTO_FEC_PERCENT, numerator of FEC percent, default 0, means no fec packats sent
    int32_t fecPercentDen; // for SRT socket option SRTO_FEC_PERCENT, denominator of FEC percent, 0 for no change, default 1
    // int64_t inputBitrate; // in bps, -1 for unlimited
    // int32_t extraBitratePercent; // extra bitrate percentage to add on top of current video bitrate, used for test bitrate change, can be negative, -1 for no change
    SrtPushNodeConfig() : srtServerPort(61017), videoSsrc(0), audioSsrc(0), dataSsrc(0), targetBitrate(-1),
        packetLossResilienceLevel(SRT_PACKET_LOSS_RESILIENCE_LEVEL_NONE), peerLatencyMs(-1), 
        fecPercentNum(0), fecPercentDen(1) {}
};

// for SRT Pull node
struct SrtPullNodeConfig : public GenericData {
    // std::set<std::string> srtServerIps; // for multi-ip, make it unique
    uint16_t srtServerPort;
    bool enablePacketLossResilience;
    SrtPullNodeConfig() : srtServerPort(61017), enablePacketLossResilience(false) {}
};

struct VideoDecoderNodeConfig : public GenericData {
    std::shared_ptr<utils::GenericObject> surface;
    std::shared_ptr<utils::GenericObject> surfaceTexture;
    uint32_t textureId; // for surfaceTexture, used by render node
    bool hasBFrames; // whether the encoded video stream has B frames, used for decoder optimization
    VideoDecoderNodeConfig() : textureId(0), hasBFrames(false) {}
};

struct VideoRenderNodeConfig : public GenericData {
    // for iOS, type: AVSampleBufferDisplayLayer*
    void* displayLayer;
    std::shared_ptr<utils::GenericObject> surface;
    int inputFormat; // expected input format, e.g., MEDIA_DATA_FORMAT_OES_TEXTURE, MEDIA_DATA_FORMAT_YUV420P, etc.
    VideoRenderNodeConfig() : displayLayer(NULL), inputFormat(MEDIA_DATA_FORMAT_UNKNOWN) {}
};

struct YUVFileSourceConfig : public GenericData {
    std::string fileName;
    int format;
    int width;
    int height;
    int fps;

    YUVFileSourceConfig() : fileName(""), 
        format(MEDIA_DATA_FORMAT_YUV420P), 
        width(640), height(480), fps(30) {
    }
};

struct H264FileSourceConfig : public GenericData {
    std::string fileName;
    std::string tsFileName;  // Optional timestamp file, each line contains one timestamp in seconds
    int32_t fps;

    H264FileSourceConfig() : fileName(""), tsFileName(""), fps(30) {
    }
};

struct PCMFileSourceConfig : public GenericData {
    std::string fileName;
    int format;
    int channels;
    int sampleRate;
    int bitsPerSample;

    PCMFileSourceConfig() : fileName(""), 
        format(MEDIA_DATA_FORMAT_PCM_S16), 
        channels(2), sampleRate(44100), bitsPerSample(16) {
    }
};

struct EncodedAudioFileSourceConfig : public GenericData {
    std::string fileName;

    EncodedAudioFileSourceConfig() : fileName("") {
    }
};

// for CameraCaptureNodeIos
struct CameraCaptureNodeConfig : public GenericData {
    int width;
    int height;
    int fps;
    int format;  // not used, for android output format is MEDIA_DATA_FORMAT_YUV420P, for ios format is MEDIA_DATA_FORMAT_NV12.

    // Optional direct-to-surface output (Android only). When surface is set, the camera
    // captures directly onto it instead of producing YUV frames via ImageReader. If
    // surfaceTexture and textureId are also set, the node additionally emits
    // MEDIA_DATA_FORMAT_OES_TEXTURE frames whenever SurfaceTexture reports a new frame,
    // mirroring VideoDecoderNodeConfig's surface/surfaceTexture/textureId convention.
    std::shared_ptr<utils::GenericObject> surface;
    std::shared_ptr<utils::GenericObject> surfaceTexture;
    uint32_t textureId;

    CameraCaptureNodeConfig() : width(1280), height(720), fps(30), format(MEDIA_DATA_FORMAT_YUV420P), textureId(0) {}
};

// for VideoScaler node
struct VideoScalerNodeConfig : public GenericData {
    int width;          // target width
    int height;         // target height
    int filterMode;     // libyuv::FilterMode: 0=None, 1=Linear, 2=Bilinear, 3=Box

    VideoScalerNodeConfig() : width(1280), height(720), filterMode(2) {}  // default to Bilinear
};

// for NdiSend node
struct NdiSendNodeConfig : public GenericData {
    std::string ndiName;    // NDI source name
    bool clockVideo;        // true: clock to video, false: clock to audio

    NdiSendNodeConfig() : ndiName("XbotGo NDI Source"), clockVideo(true) {}
};

// for NdiRecv node
struct NdiRecvNodeConfig : public GenericData {
    std::string ndiSourceName;  // NDI source name to connect to

    NdiRecvNodeConfig() : ndiSourceName("") {}
};

// for VideoFps node
struct VideoFpsNodeConfig : public GenericData {
    int fps;  // Target FPS to limit to

    VideoFpsNodeConfig() : fps(30) {}
};

// for MediaDataObserver node
struct MediaDataObserverConfig : public GenericData {
    MediaDataObserverCallback callback;  // Callback function to observe media data

    MediaDataObserverConfig() : callback(nullptr) {}
};

// for ExternalMediaNodeAdapter node (Android), wraps a Java ExternalMediaNode instance
struct ExternalMediaNodeAdapterConfig : public GenericData {
    std::shared_ptr<utils::GenericObject> externalMediaNode;
};

struct OesToGlTextureNodeConfig : public GenericData {
    void* glDisplay;
    void* glContext;
    void* pbufferSurface;  // External 1×1 PBuffer EGLSurface; if non-null the node uses it
                           // directly and does NOT destroy it in onDestroy().
    std::shared_ptr<utils::GenericObject> surfaceTexture;
    OesToGlTextureNodeConfig() : glDisplay(NULL), glContext(NULL), pbufferSurface(NULL) {}
};

}
}