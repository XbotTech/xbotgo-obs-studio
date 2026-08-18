#pragma once
#include <stdint.h>
#include <string>
#include "SignalingTypes.h"

namespace blink {
namespace signaling {

struct PeerInfo {
  // for chameleon
  std::string peerIp;
  // for dragonfly
  std::string peerId;
};

class SignalingClientEventHandler {
public:
  virtual ~SignalingClientEventHandler() {}
  virtual void onConnectionStatus(const PeerInfo &info, int state, int reason) {}
  virtual void onRecvMessage(const SignalingMessage &msg) {}
};

struct SignalingConnectConfig {
  uint32_t appVersion;
  // for chameleon
  std::string deviceName;
  std::string deviceType;
  // for falcon
  std::string deviceId; // maybe IP
  // MQTT broker IP list
  std::vector<std::string> ipv4List;
  std::vector<std::string> ipv6List;
  int32_t mqttBrokerPort;
  SignalingClientEventHandler *handler;
  SignalingConnectConfig() : appVersion(0), mqttBrokerPort(1883), handler(nullptr) {}
};

class SignalingClient {
public:
  static SignalingClient* create(SignalingProtocol protocol);
  virtual ~SignalingClient() {}

  virtual int connect(const SignalingConnectConfig& config) = 0;
  virtual int disconnect() = 0;
  virtual bool isConnected() = 0;
  virtual int sendMessage(const SignalingMessage &msg) = 0;
};

} // namespace signaling
} // namespace blink
