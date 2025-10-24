#include "rate-limiter.h"
#include <algorithm>

namespace ns3 {

RateLimiter::RateLimiter(uint32_t maxRate, uint32_t detectionThreshold)
  : m_maxRate(maxRate),
    m_detectionThreshold(detectionThreshold),
    m_defenseActive(false),
    m_activationTime(Seconds(0)),
    m_totalDropped(0),
    m_totalAllowed(0),
    m_windowStart(Seconds(0)),
    m_windowPackets(0)
{
}

bool RateLimiter::AllowPacket(Ipv4Address sourceIp, Time currentTime)
{
  // Update defense state based on aggregate traffic
  UpdateDefenseState(currentTime);

  // If defense not active, allow all packets WITHOUT consuming tokens
  if (!m_defenseActive)
  {
    m_totalAllowed++;
    return true;
  }

  // Defense is active - apply rate limiting
  // Get or create bucket for this source
  SourceBucket& bucket = m_buckets[sourceIp];

  // Initialize bucket if first packet from this source AFTER defense activated
  if (bucket.lastUpdate == Seconds(0))
  {
    bucket.tokens = m_maxRate;
    bucket.lastUpdate = currentTime;
    bucket.packetCount = 0;
    std::cout << "[DEBUG] Initialized token bucket for " << sourceIp
              << " with " << m_maxRate << " tokens" << std::endl;
  }

  // Calculate time elapsed since last update
  double elapsed = (currentTime - bucket.lastUpdate).GetSeconds();

  // Refill tokens based on elapsed time (token bucket algorithm)
  double oldTokens = bucket.tokens;
  bucket.tokens = std::min(static_cast<double>(m_maxRate),
                           bucket.tokens + (m_maxRate * elapsed));
  bucket.lastUpdate = currentTime;

  // Debug: Log token state for first few packets from each source
  static std::map<Ipv4Address, uint32_t> debugCount;
  if (debugCount[sourceIp] < 5)
  {
    std::cout << "[DEBUG] Source " << sourceIp << ": tokens " << oldTokens
              << " → " << bucket.tokens << " (elapsed=" << elapsed
              << "s, rate=" << m_maxRate << " pps)" << std::endl;
    debugCount[sourceIp]++;
  }

  // Check if we have tokens available
  if (bucket.tokens >= 1.0)
  {
    bucket.tokens -= 1.0;  // Consume one token
    bucket.packetCount++;
    m_totalAllowed++;
    return true;
  }
  else
  {
    // No tokens available - drop the packet
    m_droppedPerSource[sourceIp]++;
    m_totalDropped++;

    // Log first few drops per source
    if (m_droppedPerSource[sourceIp] <= 3)
    {
      std::cout << "[DEBUG] DROPPED packet from " << sourceIp
                << " (tokens=" << bucket.tokens << ")" << std::endl;
    }

    return false;
  }
}

void RateLimiter::UpdateDefenseState(Time currentTime)
{
  // Initialize window if first packet
  if (m_windowStart == Seconds(0))
  {
    m_windowStart = currentTime;
    std::cout << "[DEBUG] Rate limiter: First packet at t=" << currentTime.GetSeconds() << "s" << std::endl;
  }

  // Check if we need to start a new measurement window
  double elapsed = (currentTime - m_windowStart).GetSeconds();
  if (elapsed >= WINDOW_SIZE)
  {
    // Calculate current packet rate
    uint32_t currentRate = static_cast<uint32_t>(m_windowPackets / elapsed);

    // Debug output every window
    std::cout << "[DEBUG] t=" << currentTime.GetSeconds() << "s: Rate = " << currentRate
              << " pps (threshold: " << m_detectionThreshold << " pps)" << std::endl;

    // Trigger defense if rate exceeds threshold and not already active
    if (!m_defenseActive && currentRate > m_detectionThreshold)
    {
      m_defenseActive = true;
      m_activationTime = currentTime;
      std::cout << "\n*** ATTACK DETECTED at t=" << currentTime.GetSeconds()
                << "s (rate: " << currentRate << " pps) ***" << std::endl;
      std::cout << "*** DEFENSE ACTIVATED - Rate limiting enabled ***\n" << std::endl;
    }

    // Reset window
    m_windowStart = currentTime;
    m_windowPackets = 0;
  }

  // Count this packet in the current window
  m_windowPackets++;
}

uint32_t RateLimiter::GetCurrentRate() const
{
  uint32_t total = 0;
  for (const auto& bucket : m_buckets)
  {
    total += bucket.second.packetCount;
  }
  return total;
}

void RateLimiter::Reset()
{
  m_buckets.clear();
  m_droppedPerSource.clear();
  m_totalDropped = 0;
  m_totalAllowed = 0;
  m_windowStart = Seconds(0);
  m_windowPackets = 0;
  m_defenseActive = false;
  m_activationTime = Seconds(0);
}

} // namespace ns3
