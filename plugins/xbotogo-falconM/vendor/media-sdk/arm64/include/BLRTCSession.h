//
//  BLRTCSession.hpp
//  BLinkRTC
//
//  Created by ji peng on 2024/12/6.
//

#pragma once


#include <cstddef>
#include <stdint.h>
#include <vector>
#include "DataTypes.h"
#include "NodeConfigs.h"
#include "NodeMessages.h"

namespace rtcpushsdk
{

struct DeviceInfo {
    std::vector<std::string> ipv4List;
    std::vector<std::string> ipv6List;
};

enum PeerConnectionStatus
{
    PEER_CONNECTION_STATUS_DISCONNECTED = -1,
    PEER_CONNECTION_STATUS_CONNECTED = 1,
    PEER_CONNECTION_STATUS_CONNECTING = 2
};

struct MQTTMessage
{
    // 消息的主题
    std::string topic;
    // 消息的负载（即消息的内容），是一个字节数组
    void *payload;
    // 消息负载的长度（以字节为单位）
    uint32_t payloadlen;
    int32_t qos;
    bool retain;

    MQTTMessage(): payload(NULL), payloadlen(0), qos(0), retain(false) {}
};

class BLRTCSessionListener
{
public:
    virtual ~BLRTCSessionListener() {}
    /**
     * @brief 与monitor连接状态回调接口
     *
     * @param [out ] status 连接状态 1:成功  -1失败
     */
    virtual void onPeerConnection(int status) = 0;
    /**
     * @brief 接收peer端消息接口
     *
     * @param [out]message 消息内容, 上层解析topic和反序列话消息体字段
     */
    virtual void onPeerMessage(MQTTMessage& message) = 0;
    /**
     * @brief srt推流服务状态回调接口
     *
     * @param [out]status 推流服务状态，参看 NodeMessages.h SrtStreamStatusType
     * @param [out]reason 推流服务失败原因，参看 NodeMessages.h SrtDisconnectReason
     */
    virtual void onSrtStreamStatus(int status, int reason) = 0;
    virtual void onVideoEncodeConfigChange(const blink::media::VideoEncoderConfigChangeMessage &config) {}
};


struct BLRTCSessionConfig {
    blink::media::VideoEncoderNodeConfig videoEncodeConfig;
};

struct StartSessionConfig {
    // for chameleon
    std::string deviceName;
    std::string deviceType;
    uint32_t appVersion;
    // for dragonfly
    std::string deviceId; // maybe IP
    // MQTT broker IP list
    std::vector<std::string> ipv4List;
    std::vector<std::string> ipv6List;
    int32_t mqttBrokerPort;
    // supported formats for linux: MEDIA_DATA_FORMAT_H264
    // for android: MEDIA_DATA_FORMAT_YUV420P, MEDIA_DATA_FORMAT_NV21
    // for ios: MEDIA_DATA_FORMAT_IMAGE_BUFFER
    blink::media::MEDIA_DATA_FORMAT pushVideoFormat;
    StartSessionConfig() : appVersion(0), mqttBrokerPort(1883), pushVideoFormat(blink::media::MEDIA_DATA_FORMAT_YUV420P) {}
};

enum RTC_SESSION_TYPE {
    RTC_SESSION_TYPE_CHAMELEON = 0,
    RTC_SESSION_TYPE_DRAGONFLY = 1
};

class BLRTCSession
{
public:
    static BLRTCSession* create(RTC_SESSION_TYPE type, const BLRTCSessionConfig & config);

    virtual ~BLRTCSession();

    // for chameleon
    virtual int getDeviceInfo(DeviceInfo &info) = 0;
    /**
     * @brief 开启服务，服务启动时调用
     *
     * @param [in] deviceName  设备名称 ,[in] deviceType  设备类型 1:iphone,2:ipad,3:android,4:android-pad
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int startSession(const StartSessionConfig &config) = 0;
    /**
     * @brief 开启推流服务，服务启动时调用
     *
     * @param [in] url : monitor ip地址
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    // int startLive(const std::string& url);
    /**
     * @brief 停止推流服务
     */
    virtual int stopSession() = 0;
    /**
     * @brief 推流视频图像数据
     *
     * @param [in] data sensor图像数据
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int pushVideo(std::shared_ptr<blink::media::MediaData> data) = 0;

    /**
     * @brief 推流视频音频数据
     *
     * @param [in] audioBuffer 音频数据
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int pushAudio(std::shared_ptr<blink::media::MediaData> data) = 0;
    
    virtual int pushRTPMessage(std::shared_ptr<blink::media::MediaData> data) = 0;

    virtual int setVideoEncoderConfig(const blink::media::VideoEncoderConfigChangeMessage &config) = 0;
    /**
     * @brief 发送消息接口
     *
     * @param [in] topic 消息主题，payloadlen 消息体长度，payload 消息体序列化内容
     * @return 返回接口调用结果
     * @retval 0 - Success
     * @retval -1 - Failed
     */
    virtual int sendPeerMessage(const MQTTMessage &msg) = 0;
    
    /**
     * @brief 添加服务监听者
     *
     * @param [in] listener  监听回调
     */
    virtual void addListener(BLRTCSessionListener* listener) = 0;
};

}
