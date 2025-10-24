#include "rate-limited-udp-server.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(RateLimitedUdpServer);

TypeId
RateLimitedUdpServer::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::RateLimitedUdpServer")
    .SetParent<Application>()
    .SetGroupName("Applications")
    .AddConstructor<RateLimitedUdpServer>();
  return tid;
}

RateLimitedUdpServer::RateLimitedUdpServer()
  : m_socket(0),
    m_port(9),
    m_rateLimiter(nullptr),
    m_received(0),
    m_dropped(0)
{
}

RateLimitedUdpServer::~RateLimitedUdpServer()
{
  m_socket = 0;
}

void
RateLimitedUdpServer::SetRateLimiter(RateLimiter* limiter)
{
  m_rateLimiter = limiter;
}

void
RateLimitedUdpServer::SetPort(uint16_t port)
{
  m_port = port;
}

void
RateLimitedUdpServer::StartApplication(void)
{
  if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
    m_socket->Bind(local);
  }

  m_socket->SetRecvCallback(MakeCallback(&RateLimitedUdpServer::HandleRead, this));
}

void
RateLimitedUdpServer::StopApplication(void)
{
  if (m_socket != 0)
  {
    m_socket->Close();
    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
  }
}

void
RateLimitedUdpServer::HandleRead(Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom(from)))
  {
    if (InetSocketAddress::IsMatchingType(from))
    {
      InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
      Ipv4Address sourceIp = address.GetIpv4();

      // Check with rate limiter if defense is active
      bool allowed = true;
      if (m_rateLimiter != nullptr)
      {
        allowed = m_rateLimiter->AllowPacket(sourceIp, Simulator::Now());
      }

      if (allowed)
      {
        m_received++;
        // Packet accepted - count it
      }
      else
      {
        m_dropped++;
        // Packet dropped by rate limiter - don't count as received
      }
    }
  }
}

} // namespace ns3
