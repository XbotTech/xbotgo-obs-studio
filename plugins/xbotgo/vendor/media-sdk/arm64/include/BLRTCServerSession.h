//
//  BLRTCServerSession.hpp
//  BLinkRTCS
//
//  Created by ji peng on 2024/12/18.
//

#pragma once

#include "DataTypes.h"
#include "NodeConfigs.h"
#include "PullStream.h"
#include "SignalingServer.h"

namespace rtcsdk
{

enum PeerConnectionStatus
{
    PEER_CONNECTION_STATUS_DISCONNECTED = -1,
    PEER_CONNECTION_STATUS_CONNECTED = 1,
    PEER_CONNECTION_STATUS_CONNECTING = 2
};

struct BLNSPClient
{
    std::string name;
    std::string ip;
    time_t last_heartbeat_time;
    std::string device_type; // 1:iphone,2:ipad,3:android,4 android-pad
    uint32_t app_version;
    // for falcon
    std::string localIp;        // Local IP address (without scope/interface name)
    std::string remoteIp;       // Remote IP address (without scope/interface name)
    std::string localIfName;    // Local interface name (e.g., "en0")
    int localScopeId;           // Local scope ID for IPv6 link-local (0 for IPv4 or non-link-local)
    BLNSPClient() : last_heartbeat_time(0), app_version(0)
    {
    }
};


struct MQTTMessage
{
    // 消息的主题
    std::string topic;
    // 消息的负载（即消息的内容），是一个字节数组
    void *payload;
    // 消息负载的长度（以字节为单位）
    uint32_t payloadlen;
    int qos;
    bool retain;

    MQTTMessage() : payload(NULL), payloadlen(0), qos(0), retain(false)
    {
    }
};

struct PullStreamParams {
    blink::media::SrtPullNodeConfig srtPullCfg;
    blink::media::SrtPushNodeConfig srtPushCfg;
    blink::media::VideoEncoderNodeConfig videoEncodeCfg;
};

struct VideoRenderParams {
    // for android video render
    void *jsurface;
    // for ios video live output
    int32_t liveOutputWidth;
    int32_t liveOutputHeight;
    VideoRenderParams() : jsurface(NULL), liveOutputWidth(1920), liveOutputHeight(1080) {}
};

struct RemoteVideoPostDecoderParams {
    void* postDecoderNode;
};

class BLRTCServerSessionListener
{
public:
    virtual ~BLRTCServerSessionListener() {}
    /**
     * @brief 扫描capture端设备刷新回调接口
     *
     * @param [out] client 发现的设备
     */
    virtual void onPeerDevicesRefresh(BLNSPClient& client) = 0;
    /**
     * @brief capture端设备连接回调接口
     *
     * @param [out] client  [out ] status 连接状态 1:成功  -1失败
     */
    virtual void onPeerConnectStatus(BLNSPClient& client, int status) = 0;
    virtual void onEncodedFrame(uint32_t ssrc, std::shared_ptr<blink::media::MediaData> data) = 0;
    /**
     * @brief 图像解码回调接口
     *
     * @param [out] pixelBuffer  图像yuv格式buffer
     */
    virtual void onDecodedFrame(uint32_t ssrc, std::shared_ptr<blink::media::MediaData> data) = 0;
    virtual void onRTPMessage(uint32_t ssrc, std::shared_ptr<blink::media::MediaData> data) = 0;
    /**
     * @brief 接收peer端消息接口
     *
     * @param [out] client peer对象，message 消息内容, 上层解析topic和反序列话消息体字段
     */
    virtual void onPeerMessage(BLNSPClient& client,const MQTTMessage& message) = 0;

    virtual void onNewSrtStream(const blink::media::MediaStreamInfo &msg) = 0;
    virtual void onDeleteSrtStream(const blink::media::MediaStreamInfo &msg) = 0;
    virtual void onVideoFormatChanged(uint32_t ssrc, int width, int height) = 0;
    virtual void onSrtPullStates(const blink::media::SrtPullStatesMessage& msg) = 0;
};

enum RTC_SESSION_TYPE {
    RTC_SESSION_TYPE_CHAMELEON = 0,
    RTC_SESSION_TYPE_DRAGONFLY = 1
};

class BLRTCServerSession
{
public:
    static BLRTCServerSession* create(RTC_SESSION_TYPE type);

    virtual ~BLRTCServerSession();
    /**
     * @brief 开启监听服务，服务启动时调用
     */
    virtual int startSession() = 0;
    
    /**
     * @brief capture端设备连接
     *
     * @param [in] params  和设备建连的参数
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int connectPeerSession(const blink::signaling::ConnectClientConfig& params) = 0;

    /**
     * @brief 断开和capture端设备的连接
     *
     * @param [in] params  标识设备的ID，对于chameleon端设备是ip，对于dragonfly端设备是deviceId
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int disconnectPeerSession(const std::string& client) = 0;

    /**
     * @brief 停止监听服务，服务推出时调用
     */
    virtual int stopSession() = 0;
    
    /**
     * @brief 发送消息接口
     *
     * @param [in] peerIp  capture端设备ip ，topic 消息主题，payloadlen 消息体长度，payload 消息体序列化内容
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int sendPeerMessage(const std::string& client, const MQTTMessage &msg) = 0;
    
    /**
     * @brief 添加服务监听者
     *
     * @param [in] listener  监听回调
     */
    virtual void addListener(BLRTCServerSessionListener* listener) = 0;
    
    /**
     * @brief 重连接口
     *
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int reconnectPeerSession() = 0;

    virtual int addSurface(uint32_t ssrc, const VideoRenderParams &params) = 0;
    virtual int removeSurface(uint32_t ssrc) = 0;
    virtual int addRemoteAudioPlayer(uint32_t ssrc) = 0;
    virtual int removeRemoteAudioPlayer(uint32_t ssrc) = 0;

    virtual int setRemoteVideoDecodedSize(uint32_t ssrc, const blink::media::VideoScalerSizeChangeMessage &msg) = 0;
    virtual int addRemoteVideoLiveOutput(uint32_t ssrc, const blink::media::RemoteVideoLiveOutputConfig &params) = 0;
    virtual int removeRemoteVideoLiveOutput(uint32_t ssrc) = 0;

    virtual int setRemoteVideoPostDecoderParams(uint32_t ssrc, const RemoteVideoPostDecoderParams &params) = 0;

    virtual int startPullStream(const std::string &deviceId, const PullStreamParams &params) = 0;
    virtual int stopPullStream(const std::string &deviceId) = 0;
};

}
