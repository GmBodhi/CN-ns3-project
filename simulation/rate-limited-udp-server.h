#ifndef RATE_LIMITED_UDP_SERVER_H
#define RATE_LIMITED_UDP_SERVER_H

#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "rate-limiter.h"

namespace ns3 {

/**
 * RateLimitedUdpServer - UdpServer with integrated rate limiting
 * Filters packets at application level so NetAnim sees all traffic
 */
class RateLimitedUdpServer : public Application
{
public:
  static TypeId GetTypeId(void);
  RateLimitedUdpServer();
  virtual ~RateLimitedUdpServer();

  void SetRateLimiter(RateLimiter* limiter);
  void SetPort(uint16_t port);
  uint32_t GetReceived() const { return m_received; }
  uint32_t GetDropped() const { return m_dropped; }

protected:
  virtual void StartApplication(void);
  virtual void StopApplication(void);

private:
  void HandleRead(Ptr<Socket> socket);

  Ptr<Socket> m_socket;
  uint16_t m_port;
  RateLimiter* m_rateLimiter;
  uint32_t m_received;
  uint32_t m_dropped;
};

} // namespace ns3

#endif /* RATE_LIMITED_UDP_SERVER_H */
