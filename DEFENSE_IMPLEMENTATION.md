# DDoS Defense Implementation

## Overview

This implementation adds an **attack-triggered rate limiting defense** mechanism to the DDoS simulation with enhanced NetAnim visualization and separate statistics tracking for legitimate vs. attack traffic.

## New Components

### 1. Rate Limiter (`simulation/rate-limiter.h/cc`)

A token bucket-based rate limiter that:
- **Monitors aggregate traffic** to detect attack conditions (threshold: 500 pps)
- **Applies per-source rate limiting** when attack is detected (max: 100 pps per source)
- **Automatically activates** when traffic exceeds detection threshold
- **Tracks statistics** including drops per source and total allowed/dropped packets

**Key Features:**
- Token bucket algorithm for smooth rate limiting
- Per-source IP tracking
- Attack detection via sliding window measurement
- Minimal overhead when defense is inactive

### 2. Enhanced main.cc Integration

**New Command-Line Parameter:**
```bash
--enableDefense=<true|false>  # Enable/disable rate limiting defense (default: true)
```

**Defense Behavior:**
- **Passive Mode (t < attack detection)**: All packets pass through
- **Active Mode (t >= attack detection)**: Rate limiting applied per source
- **Automatic Activation**: Triggers at ~5s when attack traffic ramps up

### 3. NetAnim Visualization Enhancements

**Dynamic Visual Feedback:**
- **Router color change**: Blue (normal) → Yellow (defending)
- **Router description**: "Router (Normal)" → "Router (DEFENDING)"
- **Attacker descriptions**: Updated with dropped packet counts in real-time
- **Node labels**: Show client/attacker IDs and status
- **Packet metadata**: Enabled for detailed packet-level visualization

### 4. Enhanced Statistics Tracking

**Separate Metrics for:**
- **Legitimate Traffic**: TX/RX/success rate for normal clients
- **Attack Traffic**: TX/RX/success rate for attackers
- **Defense Statistics**: Activation time, packets dropped, packets allowed, sources rate-limited

## Testing Instructions

### 1. Start Docker Environment

```bash
# Start Docker Desktop (macOS)
# Then start the container:
docker-compose up --build
```

### 2. Build the Simulation

```bash
# Inside the container:
docker-compose exec ns3 bash

# Build:
./ns3 build
```

### 3. Test Scenarios

#### Scenario A: Baseline (No Attack, No Defense)
```bash
./ns3 run "scratch/simulation/main --enableAttack=false --enableDefense=false"
```
**Expected:** ~100% success rate for legitimate traffic

#### Scenario B: Attack Without Defense
```bash
./ns3 run "scratch/simulation/main --enableAttack=true --enableDefense=false"
```
**Expected:**
- Legitimate success rate: <20% (severe congestion)
- Attack dominates bandwidth
- Server overwhelmed

#### Scenario C: Attack With Defense (Main Test)
```bash
./ns3 run "scratch/simulation/main --enableAttack=true --enableDefense=true"
```
**Expected:**
- Defense activates at ~5-6s (when attack starts)
- Legitimate success rate: >80% (protected)
- Attack traffic rate-limited to 100 pps per attacker
- Console shows "ATTACK DETECTED" and "DEFENSE ACTIVATED" messages

#### Scenario D: Custom Parameters
```bash
./ns3 run "scratch/simulation/main --nAttackers=10 --nLegitimate=5 --simulationTime=30 --enableAttack=true --enableDefense=true"
```

### 4. Analyze Results

#### Console Output
Look for these sections:
```
--- Legitimate Traffic ---
Packets sent: XXX
Packets received: XXX
Success rate: XX.XX%

--- Attack Traffic ---
Packets sent: XXXX
Packets received: XXX
Success rate: XX.XX%

--- Defense Statistics ---
Defense activated at: 5.XXXs
Packets dropped by rate limiter: XXXX
Packets allowed: XXXX
Sources rate-limited: 5
```

#### NetAnim Visualization
```bash
# From project root:
./launch-netanim.sh

# Or manually:
cd /home/ns3/ns-allinone-3.39/netanim-3.108
./NetAnim ../../ns-3.39/ddos-simulation.xml
```

**What to observe:**
- Router turns **yellow** when defense activates (~5s)
- **Packet counters on router**: "Forwarded" and "Blocked" counts visible
- Attacker nodes show "Blocked: XXX" in descriptions
- Packets visibly **stopped at the router** when blocked (not reaching server)
- Packet flow rate visibly reduced during attack
- Legitimate client traffic continues to flow
- **No spurious traffic** after applications stop (one-way traffic model)

## Implementation Details

### Rate Limiting Algorithm

**Token Bucket:**
```
tokens = min(maxRate, tokens + (maxRate × elapsed))
if (tokens >= 1.0):
    tokens -= 1.0
    allow packet
else:
    drop packet
```

**Attack Detection:**
```
windowRate = packetsInWindow / windowDuration
if (windowRate > 500 pps && !defenseActive):
    activate defense
```

### Packet Flow

```
Client/Attacker sends packet → Arrives at router
    ↓
Packet forwarded to server link
    ↓
RxCallback() at router device → Extract source IP
    ↓
Check if defense active
    ↓ (yes)
RateLimiter::AllowPacket(sourceIP)
    ↓
Token bucket check
    ↓
Allow: Forward to server + increment counter
    ↓
Drop: Block at router + increment drop counter + update NetAnim
```

**Key improvement**: Packets are now blocked **at the router** (not at server), making the defense visible in NetAnim.

### Application Model

**Server**: `PacketSink` (one-way receiver)
- Accepts UDP packets without sending responses
- Prevents spurious echo traffic after clients stop
- More realistic for DDoS scenarios (server just receives flood)

**Clients/Attackers**: `OnOffApplication` (one-way senders)
- Sends UDP packets at configured data rate
- No echo protocol overhead
- Clean NetAnim visualization (unidirectional flow)

**Advantage**: No back-and-forth traffic means NetAnim shows only actual attack/legitimate traffic, not protocol overhead.

### Statistics Classification

Flows are classified by source IP:
- Match against `attackers` node IPs → Attack Traffic
- Match against `legitimateClients` node IPs → Legitimate Traffic

## Expected Performance Improvements

| Metric | Without Defense | With Defense | Improvement |
|--------|----------------|--------------|-------------|
| Legitimate Success Rate | 15-25% | 80-95% | **+60-70%** |
| Server Saturation | 100% | 40-60% | Reduced |
| Attack Effectiveness | High | Low | Mitigated |
| Legitimate Packet Loss | 75-85% | 5-20% | **-70%** |

## Files Modified/Created

### New Files:
- `simulation/rate-limiter.h` - Rate limiter class definition
- `simulation/rate-limiter.cc` - Rate limiter implementation
- `DEFENSE_IMPLEMENTATION.md` - This documentation

### Modified Files:
- `simulation/main.cc` - Integrated defense, NetAnim updates, enhanced statistics

## Troubleshooting

### Build Errors
```bash
# Clean rebuild:
./ns3 clean
./ns3 build
```

### Defense Not Activating
- Check that `--enableDefense=true`
- Verify attack is enabled: `--enableAttack=true`
- Check console for "ATTACK DETECTED" message
- Increase simulation time if needed

### NetAnim Not Showing Defense
- Ensure defense activated (check console)
- Look for router color change at ~5s mark
- Check node descriptions for "DEFENDING" status

## Future Enhancements

Possible improvements:
1. **Adaptive rate limits** based on server capacity
2. **Connection tracking** for TCP-based simulations
3. **IP reputation scoring** with gradual rate limit increases
4. **Priority queuing** to separate legitimate/attack traffic
5. **Geographic filtering** for distributed attacks
6. **Time-series CSV export** for plotting performance graphs

## References

- Token bucket algorithm: https://en.wikipedia.org/wiki/Token_bucket
- ns-3 Callback system: https://www.nsnam.org/docs/manual/html/callbacks.html
- NetAnim documentation: https://www.nsnam.org/wiki/NetAnim
