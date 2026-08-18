#pragma once
#include <string>
#include <stdint.h>
#include <memory>
#include "DataTypes.h"
#include "NodeConfigs.h"
#include "NodeMessages.h"
#include "worker_thread.h"
#include "MediaNode.h"

namespace blink {
namespace media {

class NdiPushStreamListener {
public:
    virtual ~NdiPushStreamListener() {}
    virtual void onNdiStreamStatus(int status, int reason) = 0;
};

struct NdiPushStreamConfig {
    NdiSendNodeConfig ndiSendConfig;
    NdiPushStreamConfig() {}
};

class INdiPushStream {
public:
    virtual ~INdiPushStream() {}

    virtual int setListener(NdiPushStreamListener *listener) = 0;
    virtual int start(const NdiPushStreamConfig &cfg) = 0;
    virtual int stop() = 0;

    virtual int pushVideo(std::shared_ptr<MediaData> data) = 0;
    virtual int pushAudio(std::shared_ptr<MediaData> data) = 0;
};

class NdiPushStream : public INdiPushStream, public MessageListener {
public:
    NdiPushStream();
    virtual ~NdiPushStream();

    int setListener(NdiPushStreamListener *listener) override;
    int start(const NdiPushStreamConfig &cfg) override;
    int stop() override;

    int pushVideo(std::shared_ptr<MediaData> data) override;
    int pushAudio(std::shared_ptr<MediaData> data) override;

    virtual int onMessage(std::shared_ptr<MediaNodeMessage> msg) override;

private:
    std::unique_ptr<MediaNode> m_ndiSend;
    std::shared_ptr<worker_thread> m_worker;
    NdiPushStreamListener *m_listener;
};

} // namespace media
} // namespace blink
