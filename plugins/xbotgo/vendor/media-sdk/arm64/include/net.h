#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <set>
#include <functional>

namespace blink {
namespace media {
namespace net {

enum SOCKET_TYPE {
  SOCKET_TYPE_TCP,
  SOCKET_TYPE_UDP
};

enum AF_ALG_TYPE {
  AF_ALG_UNSPEC = 0,
  AF_ALG_INET = 1,
  AF_ALG_INET6 = 2
};

enum NET_ERROR {
  NET_OK = 0,
  NET_ERROR = -1,
  NET_TIMEOUT = -2,
  NET_INTERRUPTED = -3
};

enum IP_ADDR_SCOPE_TYPE {
  IP_ADDR_SCOPE_UNSPEC = 0,
  IPv4_ADDR_LOOP_BACK = 1,
  IPv4_ADDR_LINK_LOCAL = 2,
  IPv4_ADDR_LOCAL_CLASS_A = 3,
  IPv4_ADDR_LOCAL_CLASS_B = 4,
  IPv4_ADDR_LOCAL_CLASS_C = 5,
  IPv4_ADDR_PUBLIC = 6,
  IPv6_ADDR_LOOP_BACK = 7,
  IPv6_ADDR_LINK_LOCAL = 8,
  IPv6_ADDR_SITE_LOCAL = 9,
  IPv6_ADDR_UNIQUE_LOCAL = 10,
  IPv6_ADDR_GLOBAL = 11,
  IP_ADDR_SCOPE_MAX
};

struct client_socket_info {
  int fd;
  struct sockaddr addr;
  socklen_t addrlen;
  client_socket_info() : fd(-1), addrlen(sizeof(addr)) {}
};

struct netcard_info {
  std::string name;
  int interface_index;
  netcard_info() : interface_index(0) {}
};

struct net_addr { 
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;
  struct sockaddr *addr;
  socklen_t addrlen;
  int family;

  net_addr();
  net_addr(struct sockaddr_in *a4);
  net_addr(struct sockaddr_in6 *a6);
  net_addr(const net_addr &other);
  net_addr& operator=(const net_addr &other);
};

int socket(SOCKET_TYPE type, AF_ALG_TYPE afType = AF_ALG_INET);
int set_socket_nonblocking(int sock);
int set_socket_reuseaddr(int sock);
int set_socket_nodelay(int sock);
int enable_keepalive(int sock);
int connect(int sock, const struct sockaddr *addr, socklen_t addrlen, int timeout_ms);
int bind(int sock, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sock, int backlog);
int accept(int sock, int timeout_ms, std::vector<client_socket_info> &client_fds);
int send(int sock, const void *buf, size_t len, int flags, int timeout_ms);
int recv(int sock, void *buf, size_t len, int flags, int timeout_ms, int interruptSock = -1);
int sendto(int sock, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen, int timeout_ms);
int recvfrom(int sock, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen, int timeout_ms, int interruptSock = -1);
int close(int sock);

int createListenSocket(int &sock, uint16_t port, AF_ALG_TYPE afType = AF_ALG_INET);
int createConnectSocket(int &sock, const std::string &ip, uint16_t port, bool keep_alive, AF_ALG_TYPE afType = AF_ALG_INET);
int createInterruptSocket(int &sock, uint16_t port);
int closeSocket(int &sock);

int sendInterruptMessage(uint16_t port);

// for ipv6 local-link address, socket is needed to get the local scope ID, in this case the ip = remote_ip%local_scope_id
int accept_addr_to_connect_string(int sock, const struct sockaddr *addr, std::string &ip, AF_ALG_TYPE afType = AF_ALG_INET);

int get_local_link_netcard(std::map<int, netcard_info> &netcard_infos,
  const std::vector<std::string>& blacklist = {},
  const std::vector<std::string>& whitelist = {});
int get_local_ips(std::vector<std::string> &ipv4List, std::vector<std::string> &ipv6List,
  const std::vector<std::string>& blacklist = {},
  const std::vector<std::string>& whitelist = {});

// input host can be like "fe80::1%eth0" for ipv6 link-local address with interface name
int ip_to_addr(const char *host, uint16_t port, net_addr *addr);
int ip_to_ifname(const char *host, std::string &localInterface);

int socket_bind_network(int fd, const char *ip);

int detect_connectivity(const std::vector<std::string> &ipList, uint16_t port, std::vector<std::string> &result, int64_t timeout);

// Helper structure to store local IP info with scope ID
struct LocalIpInfo {
  net_addr addr;       // addr6 with scope_id for IPv6 link-local address
  net_addr netmask;    // netmask
  std::string ip;      // IP address (without scope/interface name)
  std::string ifName;  // Interface name (e.g., "en0")
  int scopeId;         // For IPv6 link-local addresses
  bool isLinkLocal;    // Is this a link-local address?
  bool isPrivate;      // Is this a private/internal address?
  IP_ADDR_SCOPE_TYPE scopeType; // Address scope type

  LocalIpInfo();
  // ipStr can be ipv6 link-local address with interface name like "fe80::1%eth0"
  // LocalIpInfo(const std::string& ipStr);
  LocalIpInfo(const net_addr &netAddr,
    const net_addr &netmask,
    const std::string& ipStr, const std::string& ifname = "",
    int scope = 0, bool linkLocal = false);
  LocalIpInfo(const LocalIpInfo& other);
  LocalIpInfo& operator=(const LocalIpInfo& other);
};

int forEachLocalIp(const std::function<void(const LocalIpInfo&)>& func,
                   const std::vector<std::string>& blacklist = {},
                   const std::vector<std::string>& whitelist = {});

// Get local IP addresses with detailed info (IPv4, IPv6, IPv6 link-local)
// @param ipv4List - Output list of IPv4 addresses
// @param ipv6List - Output list of IPv6 non-link-local addresses
// @param ipv6LinkLocalList - Output list of IPv6 link-local addresses with scope info
// @param blacklist - Vector of interface name prefixes to exclude (e.g., {"lo", "docker"})
// @param whitelist - Vector of interface name prefixes to include (e.g., {"wlan", "en"})
//                    Empty vector means no whitelist filtering
void getLocalIpAddresses(std::vector<LocalIpInfo>& ipv4List,
                         std::vector<LocalIpInfo>& ipv6List,
                         std::vector<LocalIpInfo>& ipv6LinkLocalList,
                         const std::vector<std::string>& blacklist = {},
                         const std::vector<std::string>& whitelist = {});

const std::vector<std::string>& getLocalIpsWhiteList();
const std::vector<std::string>& getLocalIpsBlackList();
int getLocalIpSet(std::set<std::string> &ipSet);

// Check if an IP address is a private/internal address
// Returns true for:
//   IPv4: 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16, 127.0.0.0/8, 169.254.0.0/16
//   IPv6: fc00::/7 (ULA), fe80::/10 (link-local), ::1/128 (loopback)
// Returns false for public addresses or invalid addresses
bool isPrivateAddress(const net_addr& addr, IP_ADDR_SCOPE_TYPE &scopeType);

// Check if two IP addresses are in the same subnet
// @param localAddr - Local IP address
// @param remoteAddr - Remote IP address
// @param subnetMask - Subnet mask
// Returns true if both addresses are in the same subnet, false otherwise
bool isSameSubnet(const net_addr& localAddr, const net_addr& remoteAddr, const net_addr& subnetMask);

} // namespace net
} // namespace media
} // namespace blink
