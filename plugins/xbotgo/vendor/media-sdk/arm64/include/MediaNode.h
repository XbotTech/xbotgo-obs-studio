#pragma once
#include "DataTypes.h"
#include <memory>
#include <string>
#include <stdint.h>

namespace blink {
namespace media {


class MediaNode: public MessageListener {
public:
    virtual ~MediaNode() {}
    virtual int32_t getNodeId() const { return -1; }
    virtual std::string getNodeName() const { return ""; }
    virtual std::string getNodeType() const { return ""; }

    virtual int onCreate(const MediaNodeConfig &cfg) = 0;
    virtual int onDestroy() = 0;
    virtual int onFrame(std::shared_ptr<MediaData> data) = 0;
    virtual int addMediaSink(MEDIA_DATA_TYPE type, MediaNode *sink) = 0;
    virtual int removeMediaSink(MEDIA_DATA_TYPE type, MediaNode *sink) = 0;
};

} // namespace media
} // namespace blink
