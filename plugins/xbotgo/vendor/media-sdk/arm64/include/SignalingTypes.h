#pragma once
#include <stdint.h>
#include <string>
#include <vector>

namespace blink {
namespace signaling {

enum SignalingProtocol {
  SIGNALING_PROTOCOL_UNKNOWN = 0,
  SIGNALING_PROTOCOL_MQTT,
  SIGNALING_PROTOCOL_NSP
};

enum SignalingConnectionStatus {
  CONNECTION_STATUS_DISCONNECTED = 0,
  CONNECTION_STATUS_CONNECTING,
  CONNECTION_STATUS_CONNECTED,
  CONNECTION_STATUS_RECONNECTING,
  CONNECTION_STATUS_ERROR
};

enum SignalingDisconnectReason {
  DISCONNECT_REASON_UNKNOWN = 0,
  DISCONNECT_REASON_CONNECT_ERROR,
  DISCONNECT_REASON_READ_ERROR,
  DISCONNECT_REASON_WRITE_ERROR,
  DISCONNECT_REASON_NO_HEARTBEAT,
  DISCONNECT_REASON_CLOSE,
  DISCONNECT_REASON_PEER_CLOSE
};

struct SignalingMessage {
  int32_t messageId;
  std::string topic;
  std::vector<uint8_t> payload;
  int32_t qos;
  bool retain;

  SignalingMessage() : messageId(0), qos(0), retain(false) {}
};

} // namespace signaling
} // namespace blink