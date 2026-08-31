#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdint.h>
#include "net.h"

namespace blink {
namespace media {

// Structure to represent connection information
struct ConnectionInfo {
  std::string localIp;        // Local IP address (without scope/interface name)
  std::string remoteIp;       // Remote IP address (without scope/interface name)
  std::string localIfName;    // Local interface name (e.g., "en0")
  int localScopeId;           // Local scope ID for IPv6 link-local (0 for IPv4 or non-link-local)

  ConnectionInfo()
    : localScopeId(0) {}

  ConnectionInfo(const std::string& local, const std::string& remote,
                 const std::string& localIf = "", int localScope = 0)
    : localIp(local), remoteIp(remote)
    , localIfName(localIf), localScopeId(localScope) {}
};

// Callback function type for connectivity detection results
// Parameters:
//   - result: List of successfully connected IP pairs with detailed info
//   - error: 0 for success, -1 for stopped/cancelled, other for errors
typedef std::function<void(const std::vector<ConnectionInfo>& result, int error)> ConnectivityCallback;

/**
 * ConnectivityDetector - Detects network connectivity to remote IPs
 *
 * This class probes network connectivity to a list of remote IPs on specified ports.
 * It runs in a separate thread and reports successful connections through a callback.
 *
 * Features:
 * - Supports both IPv4 and IPv6
 * - Binds to local IPs for testing connectivity from specific interfaces
 * - Continuous probing until success or manual stop
 * - Asynchronous callback notification
 * - Thread-safe operations
 */
class ConnectivityDetector {
public:
  ConnectivityDetector();
  ~ConnectivityDetector();

  /**
   * Start connectivity detection
   *
   * @param remoteIps - List of remote IP addresses to probe
   * @param port - Port number to connect to
   * @param callback - Callback function to notify results
   * @param timeout - Timeout in milliseconds for each connection attempt (default: 5000ms)
   * @param retryInterval - Interval in milliseconds between retry rounds (default: 1000ms)
   * @return 0 on success, -1 on error (e.g., already running)
   */
  int start(const std::vector<std::string>& remoteIps,
            uint16_t port,
            ConnectivityCallback callback,
            int64_t timeout = 5000,
            int64_t retryInterval = 1000);

  /**
   * Stop connectivity detection
   *
   * Stops the detection thread gracefully. The callback will be invoked
   * with error code -1 to indicate cancellation.
   */
  void stop();

  /**
   * Check if detector is currently running
   *
   * @return true if detection is in progress, false otherwise
   */
  bool isRunning() const;

  /**
   * Set interface whitelist for local IP filtering
   *
   * @param whitelist - Vector of interface name prefixes to include (e.g., {"wlan", "en"})
   *                    Empty vector means no whitelist filtering
   */
  void setInterfaceWhitelist(const std::vector<std::string>& whitelist);

  /**
   * Set interface blacklist for local IP filtering
   *
   * @param blacklist - Vector of interface name prefixes to exclude (e.g., {"lo", "docker"})
   *                    Empty vector means no blacklist filtering
   */
  void setInterfaceBlacklist(const std::vector<std::string>& blacklist);

private:
  // Use LocalIpInfo from net namespace
  typedef net::LocalIpInfo LocalIpInfo;
  struct RemoteAddrInfo {
    std::string ipStr; // without scope/interface name
    net::net_addr addr;
    bool isPrivate;    // true if this is a private/internal address
    net::IP_ADDR_SCOPE_TYPE scopeType; // Scope type for the remote address

    RemoteAddrInfo() : isPrivate(false), scopeType(net::IP_ADDR_SCOPE_UNSPEC) {}
  };

  // Main detection thread function
  void detectionThread();

  // Perform one round of connectivity detection
  // Returns true if at least one connection succeeded
  bool detectOnce(std::vector<ConnectionInfo>& result);

  // Create pipe for interrupting select
  int createInterruptPipe();

  // Close interrupt pipe
  void closeInterruptPipe();

  // Send interrupt signal to wake up select
  void interruptSelect();

  void getLocalIps();

  // Detection thread
  std::unique_ptr<std::thread> thread_;

  // Interrupt pipe for breaking out of select
  int interruptPipe_[2];  // [0] = read end, [1] = write end

  // Synchronization primitives
  std::mutex mtx_;
  std::condition_variable cv_;
  std::atomic<bool> running_;
  std::atomic<bool> stopRequested_;

  // Detection parameters
  // std::vector<std::string> remoteIps_;
  std::vector<RemoteAddrInfo> remoteAddrs_;
  uint16_t port_;
  ConnectivityCallback callback_;
  int64_t timeout_;
  int64_t retryInterval_;

  // Cached local IP lists (initialized once per detection session)
  std::vector<LocalIpInfo> localIpv4List_;
  std::vector<LocalIpInfo> localIpv6List_;
  std::vector<LocalIpInfo> localIpv6LinkLocalList_;
  std::vector<LocalIpInfo> allLocalIps_;  // Complete list including loopback

  // Temporary buffer for local IPs to test (reused across detection rounds)
  std::vector<LocalIpInfo> localIpsToTest_;

  // Interface filtering
  std::vector<std::string> interfaceWhitelist_;
  std::vector<std::string> interfaceBlacklist_;

  // Thread name for debugging
  static const char* TAG;
};

} // namespace media
} // namespace blink
