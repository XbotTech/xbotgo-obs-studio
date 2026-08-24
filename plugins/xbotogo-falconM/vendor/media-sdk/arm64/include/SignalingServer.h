#pragma once
#include "SignalingTypes.h"
#include <cstdint>

namespace blink {
namespace signaling {

struct SignalingClientInfo {
  std::string clientId;
  // for chameleon
  std::string deviceName;
  std::string deviceType;
  int64_t lastHeartbeatTime;
  uint32_t appVersion;
  // for falcon
  std::string localIp;        // Local IP address (without scope/interface name)
  std::string remoteIp;       // Remote IP address (without scope/interface name)
  std::string localIfName;    // Local interface name (e.g., "en0")
  int localScopeId;           // Local scope ID for IPv6 link-local (0 for IPv4 or non-link-local)
  std::vector<uint8_t> connectPayload; // Payload carried on _connect/_conack handshake
  SignalingClientInfo() : lastHeartbeatTime(0), appVersion(0), localScopeId(0) {}
};

class SignalingServerEventHandler {
public:
  virtual ~SignalingServerEventHandler() {}
  virtual void onNewConnection(const SignalingClientInfo &info) {}
  virtual void onConnectionStatus(const SignalingClientInfo &info, int state, int reason) {}
  virtual void onRecvMessage(const SignalingClientInfo &info, const SignalingMessage &msg) {}
};

struct SignalingServerStartConfig {
  SignalingServerEventHandler *handler;
};

enum HANDSHAKE_PROTOCOL_TYPE {
    HANDSHAKE_PROTOCOL_UDP = 0,
    HANDSHAKE_PROTOCOL_TCP = 1
};

struct ConnectClientConfig {
    // for chameleon
    int handshakeProtocol;
    // for chameleon udp
    std::string peerIp;
    // for chameleon tcp
    std::string deviceName;
    std::string deviceType;
    // for falcon
    std::string controlerId;
    std::string deviceId;
    // std::string mqttBrokerIp;
    int32_t mqttBrokerPort;
    // for chameleon and falcon(MQTT broker IP)
    std::vector<std::string> ipv4List;
    std::vector<std::string> ipv6List;
    std::vector<uint8_t> connectPayload; // Payload carried on _connect/_conack handshake
    ConnectClientConfig() : handshakeProtocol(HANDSHAKE_PROTOCOL_UDP), mqttBrokerPort(1883) {
    }
};

class SignalingServer {
public:
  static SignalingServer* create(SignalingProtocol protocol);
  virtual ~SignalingServer() {};
  virtual int start(const SignalingServerStartConfig& config) = 0;
  virtual int stop() = 0;
  virtual int acceptClient(const ConnectClientConfig &config) = 0;
  virtual int closeClient(const std::string &clientId) = 0;
  virtual int sendMessage(const std::string &clientId, const SignalingMessage &msg) = 0;
  virtual int getClientInfo(const std::string &clientId, SignalingClientInfo &info) = 0;
};

} // namespace signaling
} // namespace blink
