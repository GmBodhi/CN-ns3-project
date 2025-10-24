#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include <map>

namespace ns3 {

/**
 * RateLimiter - Token bucket based rate limiter for DDoS defense
 *
 * Implements per-source IP rate limiting with attack detection
 */
class RateLimiter
{
public:
  /**
   * Constructor
   * \param maxRate Maximum packets per second allowed per source
   * \param detectionThreshold Aggregate pps threshold to trigger defense
   */
  RateLimiter(uint32_t maxRate = 100, uint32_t detectionThreshold = 500);

  /**
   * Check if a packet should be allowed or dropped
   * \param sourceIp Source IP address of the packet
   * \param currentTime Current simulation time
   * \return true if packet allowed, false if dropped
   */
  bool AllowPacket(Ipv4Address sourceIp, Time currentTime);

  /**
   * Check if defense is currently active
   * \return true if defense is triggered
   */
  bool IsDefenseActive() const { return m_defenseActive; }

  /**
   * Get time when defense was activated
   * \return activation time
   */
  Time GetActivationTime() const { return m_activationTime; }

  /**
   * Get statistics
   */
  uint32_t GetTotalDropped() const { return m_totalDropped; }
  uint32_t GetTotalAllowed() const { return m_totalAllowed; }
  uint32_t GetCurrentRate() const;

  /**
   * Get per-source statistics
   */
  std::map<Ipv4Address, uint32_t> GetSourceDropCounts() const { return m_droppedPerSource; }

  /**
   * Reset statistics
   */
  void Reset();

private:
  /**
   * Update defense state based on current traffic
   */
  void UpdateDefenseState(Time currentTime);

  /**
   * Token bucket data for each source
   */
  struct SourceBucket
  {
    double tokens;          // Current token count
    Time lastUpdate;        // Last update time
    uint32_t packetCount;   // Total packets from this source
  };

  uint32_t m_maxRate;                              // Max packets per second per source
  uint32_t m_detectionThreshold;                    // Attack detection threshold (aggregate pps)
  bool m_defenseActive;                             // Is defense currently active
  Time m_activationTime;                            // When defense was activated

  std::map<Ipv4Address, SourceBucket> m_buckets;   // Per-source token buckets
  std::map<Ipv4Address, uint32_t> m_droppedPerSource; // Packets dropped per source

  uint32_t m_totalDropped;                          // Total packets dropped
  uint32_t m_totalAllowed;                          // Total packets allowed

  Time m_windowStart;                               // Start of current measurement window
  uint32_t m_windowPackets;                         // Packets in current window
  static constexpr double WINDOW_SIZE = 1.0;       // 1 second measurement window
};

} // namespace ns3

#endif /* RATE_LIMITER_H */
