#pragma once
#include "worker_thread.h"
#include "PushStream.h"

namespace blink {
namespace media {

class RKPushStream : public IPushStream, public blink::media::MessageListener
{
public:
    RKPushStream();
    virtual ~RKPushStream();

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