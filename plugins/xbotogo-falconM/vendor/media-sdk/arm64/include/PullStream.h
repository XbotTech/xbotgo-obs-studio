#pragma once
#include <string>
#include <stdint.h>
#include <memory>
#include <map>
#include "DataTypes.h"
#include "MediaNode.h"
#include "NodeConfigs.h"
#include "worker_thread.h"
#include "NodeMessages.h"
#include "MediaNodeBase.h"
#include "MediaNodeGroup.h"
#include "timer.h"
#include "GlTypes.h"

namespace blink {
namespace media {

struct MediaStreamInfo {
    uint32_t ssrc;
    int mediaType;
    int mediaFormat;
    MediaStreamInfo() : ssrc(0), 
        mediaType(MEDIA_DATA_TYPE_UNKNOWN),
        mediaFormat(MEDIA_DATA_FORMAT_UNKNOWN) {}
};

struct PullStreamConfig {
    SrtPullNodeConfig srtPullConfig;
    PullStreamConfig() {}
};

struct RemoteVideoPipelineConfig {
    // for android video render
    VideoRenderNodeConfig videoRenderConfig;
    // for ios video live output
    VideoScalerNodeConfig videoScalerConfig;
    RemoteVideoPipelineConfig() {}
};

struct RemoteVideoLiveOutputConfig {
    VideoEncoderNodeConfig videoEncoderConfig;
    RemoteVideoLiveOutputConfig() {}
};

struct RemoteVideoPostDecoderConfig {
    ExternalMediaNodeAdapterConfig externalMediaNodeConfig;
    RemoteVideoPostDecoderConfig() {}
};

class PullStreamListener {
public:
    virtual ~PullStreamListener() {}
    virtual void onNewSrtStream(const MediaStreamInfo &msg) = 0;
    virtual void onDeleteSrtStream(const MediaStreamInfo &msg) = 0;
    virtual void onEncodedFrame(uint32_t ssrc, std::shared_ptr<MediaData> data) = 0;
    virtual void onDecodedFrame(uint32_t ssrc, std::shared_ptr<MediaData> data) = 0;
    virtual void onVideoFormatChanged(uint32_t ssrc, int width, int height) = 0;
    virtual void onSrtPullStates(const SrtPullStatesMessage& msg) = 0;
};

class PullStream : public MessageListener {
public:
    struct RemoteMediaGroupInfo {
        uint32_t ssrc;
        std::shared_ptr<MediaNodeGroup> mediaNodeGroup;
        // the node after SrtPullNode, for removeMediaSink
        MediaNode* headNode;

        // for android video
        std::shared_ptr<MediaNodeGroup> videoPostDecoderGroup;
        std::shared_ptr<worker_thread> videoPostDecoderWorker;
        GlContext videoPostDecoderContext;
        OesSurfaceBundle videoPostDecoderSurface;
        void* videoPostDecoderPbufferSurface;
        // for android video animation render
        MediaNode* externalNode;
        MediaNode* tailNode;

        // for android render output
        std::shared_ptr<MediaNodeGroup> videoRenderGroup;
        std::shared_ptr<worker_thread> videoRenderWorker;
        GlContext videoRenderContext;
        void* videoRenderPbufferSurface;
        MediaNode* videoRenderNode;
        // for android live output
        std::shared_ptr<MediaNodeGroup> videoEncoderGroup;
        std::shared_ptr<worker_thread> videoEncoderWorker;
        GlContext videoEncoderContext;
        void* videoEncoderPbufferSurface;
        MediaNode* videoEncoderNode;

        // for ios video pipeline only
        MediaNode* scalerNode;

        // for audio or video on linux, to determine whether decoder was created for this ssrc or not
        int32_t decoderNodeId;
        RemoteMediaGroupInfo() : ssrc(0), headNode(nullptr),
            videoPostDecoderPbufferSurface(nullptr), externalNode(nullptr), tailNode(nullptr),
            videoRenderPbufferSurface(nullptr), videoRenderNode(nullptr), 
            videoEncoderPbufferSurface(nullptr), videoEncoderNode(nullptr),
            scalerNode(nullptr), decoderNodeId(-1) {}
    };

    PullStream();
    ~PullStream();

    int setListener(PullStreamListener *listener);
    int start(const PullStreamConfig &cfg);
    int stop();

    int updatePullStreamConfig(const PullStreamConfig &cfg);

    int addRemoteVideoPipeline(uint32_t ssrc, const RemoteVideoPipelineConfig &cfg);
    int removeRemoteVideoPipeline(uint32_t ssrc);

    int addRemoteVideoPipelineIos(uint32_t ssrc, const RemoteVideoPipelineConfig &cfg);
    int removeRemoteVideoPipelineIos(uint32_t ssrc);
    int setRemoteVideoDecodedSizeIos(uint32_t ssrc, const VideoScalerSizeChangeMessage &msg);

    int addRemoteVideoRenderAndroid(uint32_t ssrc, const RemoteVideoPipelineConfig &cfg);
    int removeRemoteVideoRenderAndroid(uint32_t ssrc);

    int addRemoteVideoLiveOutputAndroid(uint32_t ssrc, const RemoteVideoLiveOutputConfig &cfg);
    int removeRemoteVideoLiveOutputAndroid(uint32_t ssrc);

    int addRemoteVideoDecoderAndroid(uint32_t ssrc);
    int removeRemoteVideoDecoderAndroid(uint32_t ssrc);

    int setRemoteVideoPostDecoderNodeAndroid(uint32_t ssrc, const RemoteVideoPostDecoderConfig &cfg);

    int addRemoteAudioPipeline(uint32_t ssrc);
    int removeRemoteAudioPipeline(uint32_t ssrc);

    int addRTPMessageDecoder(uint32_t ssrc);
    int removeRTPMessageDecoder(uint32_t ssrc);

    virtual int onMessage(std::shared_ptr<MediaNodeMessage> msg) override;
private:
    void onVideoDecoderStatesMessage(std::shared_ptr<MediaNodeMessage> msg);
    void onAudioDecoderStatesMessage(std::shared_ptr<MediaNodeMessage> msg);
    void onSrtPullStatesMessage(std::shared_ptr<MediaNodeMessage> msg);
    // std::unique_ptr<MediaNode> m_videoEncoder;
    int sendAddSrtDecodeMessage(uint32_t ssrc, int decoderNodeId);
    int sendRemoveSrtDecodeMessage(uint32_t ssrc);
    int addEncodedFrameObserver(uint32_t ssrc, MEDIA_DATA_TYPE mediaType);
    int removeEncodedFrameObserver(uint32_t ssrc, MEDIA_DATA_TYPE mediaType);
    bool createGlRenderResources(std::shared_ptr<worker_thread> worker,
                                 GlContext &glCtx, void* &pbufferSurface,
                                 OesSurfaceBundle* oesSurface,
                                 GlContext shareContext = GlContext());
    void destroyGlRenderResources(std::shared_ptr<worker_thread> worker,
                                  GlContext &glCtx, void* &pbufferSurface,
                                  OesSurfaceBundle* oesSurface);

    int m_nodeId;
    std::unique_ptr<MediaNode> m_srtPull;
    std::shared_ptr<worker_thread> m_worker;
    PullStreamListener *m_listener;
    std::map<uint32_t, RemoteMediaGroupInfo> m_videoDecoders;
    std::map<uint32_t, RemoteMediaGroupInfo> m_audioDecoders;
    std::map<uint32_t, RemoteMediaGroupInfo> m_rtpMsgDecoders;
    // map ssrc to head MediaNode pointer of remote MediaNodeGroup
    // std::map<uint32_t, MediaNode*> m_ssrcToHeadNodeMap;
    // for onMessage callback, to convert ssrc to streamInfo
    // std::map<uint32_t, MediaStreamInfo> m_ssrcToStreamInfo;

    // for states message
    blink::utils::Timer m_stateTimer;
    std::map<std::string, int64_t> m_lastStateTimeMs;
};

} // namespace media
} // namespace blink
