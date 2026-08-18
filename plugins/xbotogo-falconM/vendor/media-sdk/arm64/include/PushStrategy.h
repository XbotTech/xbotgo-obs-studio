#pragma once
#include "NodeMessages.h"

namespace blink {
namespace media {

class IPushStream;
struct PushStreamConfig;

class IPushStrategy {
public:
    virtual ~IPushStrategy() {}
    // all these callback functions will be called in push stream's worker thread
    // set pusher to strategy for strategy to control the push stream, e.g., call setVideoEncoderConfig/setTargetBitrate etc.
    // modify VideoEncodeConfig and target bitrate in place, push stream will apply the new config in next push.
    virtual int onPushStreamStart(IPushStream *pusher, PushStreamConfig &cfg) = 0;
    // reset strategy internal states.
    virtual int onPushStreamStop() = 0;
    // call push stream's setVideoEncoderConfig or setTargetBitrate to adjust bitrate according to network states.
    virtual int onSrtPushStates(const SrtPushStatesMessage &msg) = 0;
};

} // namespace media
} // namespace blink
