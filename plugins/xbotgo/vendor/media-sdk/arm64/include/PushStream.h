#pragma once
#include <string>
#include <stdint.h>
#include <memory>
#include <map>
#include "DataTypes.h"
#include "NodeConfigs.h"
#include "NodeMessages.h"
#include "worker_thread.h"
#include "MediaNode.h"
#include "timer.h"
#include "PushStrategy.h"

namespace blink {
namespace media {



class PushStreamListener {
public:
    virtual ~PushStreamListener() {}
    virtual void onSrtStreamStatus(int status, int reason) = 0;
    virtual void onVideoEncodeConfigChange(const VideoEncoderConfigChangeMessage &config) {}
};

struct PushStreamConfig {
    VideoEncoderNodeConfig videoEncodeConfig;
    AudioEncoderNodeConfig audioEncodeConfig;
    SrtPushNodeConfig srtPushConfig;
    PushStreamConfig() {}
};

class IPushStream {
public:
    virtual ~IPushStream() {}

    virtual int setListener(PushStreamListener *listener) = 0;
    virtual int setPushStrategy(IPushStrategy *pushStrategy) = 0;
    virtual int start(const PushStreamConfig &cfg) = 0;
    virtual int stop() = 0;

    virtual int pushVideo(std::shared_ptr<MediaData> data) = 0;
    virtual int pushAudio(std::shared_ptr<MediaData> data) = 0;
    virtual int pushRTPMessage(std::shared_ptr<MediaData> data) = 0;

    virtual int setVideoEncoderConfig(const VideoEncoderConfigChangeMessage &config) = 0;
    virtual int setTargetBitrate(int32_t bitrate) = 0;
    virtual int setFecPercent(int32_t percentNum, int32_t percentDen) = 0;
};

class PushStream : public IPushStream, public MessageListener {
public:
    PushStream();
    virtual ~PushStream();

    int setListener(PushStreamListener *listener) override;
    int setPushStrategy(IPushStrategy *pushStrategy) override;
    int start(const PushStreamConfig &cfg) override;
    int stop() override;

    int pushVideo(std::shared_ptr<MediaData> data) override;
    int pushAudio(std::shared_ptr<MediaData> data) override;
    int pushRTPMessage(std::shared_ptr<MediaData> data) override;

    int setVideoEncoderConfig(const VideoEncoderConfigChangeMessage &config) override;
    int setTargetBitrate(int32_t bitrate) override;
    int setFecPercent(int32_t percentNum, int32_t percentDen) override;
    virtual int onMessage(std::shared_ptr<MediaNodeMessage> msg) override;
private:
    void onSrtPushStatesMessage(std::shared_ptr<MediaNodeMessage> msg);
    void onVideoEncoderStatesMessage(std::shared_ptr<MediaNodeMessage> msg);

    std::unique_ptr<MediaNode> m_videoFps;
    std::unique_ptr<MediaNode> m_videoScaler;
    std::unique_ptr<MediaNode> m_videoEncoder;
    std::unique_ptr<MediaNode> m_audioEncoder;
    std::unique_ptr<MediaNode> m_rtpMsgEncoder;
    std::unique_ptr<MediaNode> m_srtPush;
    std::shared_ptr<worker_thread> m_worker;
    PushStreamListener *m_listener;
    IPushStrategy *m_pushStrategy;

    // for states message
    blink::utils::Timer m_stateTimer;
    std::map<std::string, int64_t> m_lastStateTimeMs;
};

} // namespace media
} // namespace blink
