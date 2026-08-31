#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <mutex>
#include <memory>
#include "ConnectivityDetector.h"

struct mosquitto;
struct mosquitto_message;

namespace blink {

// namespace media {
//   class ConnectivityDetector;
//   struct ConnectionInfo;
// }

namespace signaling {

struct MqttMessage{
  // mid is setted by mqtt client, not setted by user.
	int mid;
	std::string topic;
	std::vector<uint8_t> payload;
	int qos;
	bool retain;
};


class MqttClient {
public:
  typedef std::function<void(int)> onConnectResultFunc;
  typedef std::function<void(int)> onDisonnectResultFunc;
  typedef std::function<void()> onPublishedFunc;
  typedef std::function<void(int)> onSubscribedFunc;
  typedef std::function<void()> onUnsubscribedFunc;
  typedef std::function<void(const MqttMessage&)> onMessageFunc;

  MqttClient();
  ~MqttClient();
  int connect(const std::vector<std::string>& hosts, int port, const std::string& username = "", const std::string& password = "");
  int disconnect();
  bool isConnected();
  // std::string getLocalIp();
  int getConnectionInfo(media::ConnectionInfo& info);

  int publish(MqttMessage &msg, onPublishedFunc cb = nullptr);
  int subscribe(const std::string& topic, int qos, onSubscribedFunc cb = nullptr);
  int unsubscribe(const std::string& topic, onUnsubscribedFunc cb = nullptr);

  void onConnectResult(onConnectResultFunc func);
  void onDisonnectResult(onDisonnectResultFunc func);
  void onMessage(onMessageFunc func);
private:
  int performMqttConnect(const std::string& host, int port, const std::string& bindAddr);

  static void onConnectResultCb(struct mosquitto *mosq, void *obj, int rc);
  static void onDisonnectResultCb(struct mosquitto *mosq, void *obj, int rc);
  static void onPublishedCb(struct mosquitto *mosq, void *obj, int mid);
  static void onSubscribedCb(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
  static void onUnsubscribedCb(struct mosquitto *mosq, void *obj, int mid);
  static void onMessageCb(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
  static void onLogCb(struct mosquitto *mosq, void *obj, int level, const char *str);

  // void getLocalIpBySocket();

  struct mosquitto *mosq_;
  bool connected_;
  // std::string localIp_;
  // bool isIpv6_;
  media::ConnectionInfo connectionInfo_;

  // For async connectivity detection
  std::unique_ptr<media::ConnectivityDetector> connectivityDetector_;
  std::vector<std::string> pendingHosts_;
  int pendingPort_;
  std::string pendingUsername_;
  std::string pendingPassword_;

  std::mutex mutex_;
  onConnectResultFunc onConnectResultFunc_;
  onDisonnectResultFunc onDisonnectResultFunc_;
  onMessageFunc onMessageFunc_;

  std::map<int, onPublishedFunc> onPublishedFuncMap_;
  std::map<int, onSubscribedFunc> onSubscribedFuncMap_;
  std::map<int, onUnsubscribedFunc> onUnsubscribedFuncMap_;
};

}
}