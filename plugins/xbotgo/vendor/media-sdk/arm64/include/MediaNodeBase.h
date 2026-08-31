#pragma once
#include "MediaNode.h"
#include <cstdint>
#include <map>
#include <memory>
#include <array>
#include <utility>
#include <string>
#include <fstream>

namespace blink {
namespace media {

class MediaNodeBase : public MediaNode {
public:
    typedef std::map<int32_t, MediaNode*> MediaSinkMap;
    MediaNodeBase() : MediaNode() {}
    virtual ~MediaNodeBase() {}
    virtual int32_t getNodeId() const override { return m_config.nodeId; }
    virtual std::string getNodeName() const override { return m_config.nodeName; }
    virtual std::string getNodeType() const override { return m_config.nodeType; }

    virtual int addMediaSink(MEDIA_DATA_TYPE type, MediaNode* sink) override;
    virtual int removeMediaSink(MEDIA_DATA_TYPE type, MediaNode *sink) override;

    virtual int onCreate(const MediaNodeConfig &cfg) override;
    virtual int onDestroy() override;
    virtual int onFrame(std::shared_ptr<MediaData> data) override {
        return 0;
    }
    virtual int onMessage(std::shared_ptr<MediaNodeMessage> msg) override {
        return 0;
    }

    // send data to next node
    virtual int sendFrame(std::shared_ptr<MediaData> data);
    // send message to pipeline
    virtual int sendMessage(std::shared_ptr<MediaNodeMessage> msg);

    std::pair<int64_t, int64_t> getLastOutFramePts(MEDIA_DATA_TYPE type);
    // int64_t m__outFrames;
protected:
    MediaNodeConfig m_config;
    std::array<MediaSinkMap, MEDIA_DATA_TYPE_NUM> m_mediaSinks;
    MessageListener* m_pipeline;
    std::array<std::pair<int64_t, int64_t>, MEDIA_DATA_TYPE_NUM> m__lastOutFramePts;

    // Out frame info statistics, keyed by "<type>_<sinkNodeName>"
    std::map<std::string, std::shared_ptr<std::ofstream>> m_outFrameInfoCsvs;
};

}
}
