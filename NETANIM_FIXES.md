# NetAnim Visualization Fixes

## Issues Fixed

### ✅ Issue 1: Packets from attackers not visible when blocked
**Problem**: When defense was active, dropped packets disappeared before reaching the router in NetAnim.

**Root Cause**: Callback was attached to server device (index 1), so packets were blocked after passing through router.

**Fix**: Moved callback to router device (index 0 of serverLink):
```cpp
// Before:
Ptr<NetDevice> serverDevice = serverLink.Get(1);  // Server side

// After:
Ptr<NetDevice> routerDevice = serverLink.Get(0);  // Router side
```

**Result**: Packets now visibly arrive at router and are blocked there, showing defense in action.

---

### ✅ Issue 2: Router and server exchanging packets after clients stop
**Problem**: Router and server continued exchanging packets even after attack/client applications stopped.

**Root Cause**: Using `UdpEchoServer` and `UdpEchoClient` which create bidirectional echo protocol traffic.

**Fix**: Changed to one-way traffic model:

**Server**:
```cpp
// Before: UdpEchoServer (sends responses back)
UdpEchoServerHelper echoServer(port);

// After: PacketSink (just receives)
PacketSinkHelper sinkServer("ns3::UdpSocketFactory", ...);
```

**Clients/Attackers**:
```cpp
// Before: UdpEchoClient (expects echo responses)
UdpEchoClientHelper client(serverIP, port);
client.SetAttribute("MaxPackets", UintegerValue(50));
client.SetAttribute("Interval", TimeValue(Seconds(0.2)));

// After: OnOffApplication (just sends)
OnOffHelper client("ns3::UdpSocketFactory", ...);
client.SetAttribute("DataRate", StringValue("10Kbps"));
```

**Result**: Clean unidirectional traffic flow in NetAnim. No spurious packets after applications stop.

---

### ✅ Issue 3: No visual feedback for defense actions
**Problem**: Defense was working but not visually apparent in NetAnim.

**Fixes Applied**:

1. **Router color change**:
   ```cpp
   g_anim->UpdateNodeColor(g_routerNode, 255, 255, 0);  // Blue → Yellow
   ```

2. **Node counters** on router:
   ```cpp
   anim.AddNodeCounter(0, "Forwarded");  // Allowed packets
   anim.AddNodeCounter(1, "Blocked");    // Dropped packets

   // Updated in real-time:
   g_anim->UpdateNodeCounter(g_routerNode, 0, g_routerForwardCounter);
   g_anim->UpdateNodeCounter(g_routerNode, 1, g_routerDropCounter);
   ```

3. **Attacker node descriptions**:
   ```cpp
   g_anim->UpdateNodeDescription(attackers.Get(i),
       "Attacker " + std::to_string(i) + " (Blocked: " + std::to_string(drops) + ")");
   ```

4. **Protocol filtering** to avoid counting ARP traffic:
   ```cpp
   if (protocol != 0x0800)  // Only filter IPv4
     return true;  // Allow ARP, etc.
   ```

**Result**:
- Router visibly changes color when defending
- Real-time counters show forwarded vs. blocked packets
- Attacker nodes show how many of their packets were blocked

---

## NetAnim Visualization Features

When you open `ddos-simulation.xml` in NetAnim, you'll see:

### Color Coding
- 🔴 **Red nodes**: Attackers sending flood traffic
- 🟢 **Green nodes**: Legitimate clients with normal traffic
- 🔵 **Blue router** (0-5s): Normal operation
- 🟡 **Yellow router** (5s+): Defense active, blocking packets
- 🟠 **Orange node**: Target server

### Real-time Information

**Router node displays**:
- "Router (Normal)" → changes to "Router (DEFENDING)" at ~5s
- Counter 0: Forwarded packets count
- Counter 1: Blocked packets count

**Attacker nodes display**:
- "Attacker 0" → changes to "Attacker 0 (Blocked: 1234)"

**Client nodes display**:
- "Client 0" (remains stable)

### Traffic Patterns

**Timeline**:
- **0-2s**: Server starts, network idle
- **2-5s**: Green packets from legitimate clients → router → server (baseline)
- **5s**: Attack begins - massive red packet flood
- **~5-6s**: Router turns yellow, defense triggers, "ATTACK DETECTED" in console
- **5-15s**: Red packets visible but many blocked at router (don't reach server)
- **15s+**: Attack stops, only legitimate green traffic continues

### Packet Flow Visualization

**Without defense**:
```
Attacker → Router → Server (all packets get through)
Client   → Router → Server (but congested, many dropped)
```

**With defense**:
```
Attacker → Router ⊗ (blocked here, don't reach server)
Client   → Router → Server (protected, most get through)
```

The ⊗ symbol represents packets being dropped at the router, visible as packets that reach the router but don't continue to the server.

---

## Comparison: Before vs After

| Aspect | Before Fixes | After Fixes |
|--------|-------------|-------------|
| **Dropped packets** | Invisible (disappeared) | Visible at router before being blocked |
| **Defense location** | Server device | Router device (correct) |
| **Spurious traffic** | Echo responses continue | Clean stop when apps finish |
| **Visual feedback** | Router color only | Color + counters + descriptions |
| **Protocol overhead** | Echo protocol visible | One-way UDP only |
| **Counter updates** | None | Real-time forwarded/blocked counts |

---

## Testing the Visualization

```bash
# Build and run with defense
./ns3 clean && ./ns3 build
./ns3 run "scratch/simulation/main --enableAttack=true --enableDefense=true"

# Open in NetAnim
./launch-netanim.sh
```

### What to Check in NetAnim:

1. **Playback speed**: Set to 1x or 0.1x to see packet flow clearly
2. **Router color**: Should change from blue to yellow at ~5-6 seconds
3. **Packet paths**: Watch red packets (attackers) stop at router instead of reaching server
4. **Counters**: Click on router node to see Forwarded/Blocked statistics
5. **Node descriptions**: Hover over attacker nodes to see blocked counts
6. **Traffic cessation**: At 15s, attack traffic should cleanly stop (no lingering packets)

---

## Code Changes Summary

### Files Modified:
- `simulation/main.cc`:
  - Changed callback attachment point (line 154)
  - Added protocol filtering (line 35-37)
  - Added packet counters (lines 20-21, 62-70)
  - Changed server: UdpEchoServer → PacketSink (line 174)
  - Changed clients: UdpEchoClient → OnOffApplication (lines 181, 195)
  - Added NetAnim counter setup (lines 228-232)

### Lines of Code Changed: ~80 lines
### New Features Added: 4
### Bugs Fixed: 3

---

## Performance Impact

These changes have **zero performance overhead**:
- Protocol filtering is a simple integer comparison
- Counter increments are O(1) operations
- One-way traffic reduces packet count (no echo responses)
- NetAnim updates happen only when defense is active

---

## Troubleshooting

**Q: Packets still showing after apps stop**
- A: Make sure you've rebuilt after changes: `./ns3 clean && ./ns3 build`

**Q: Counters not showing in NetAnim**
- A: Counters only appear when `--enableDefense=true`
- Check that defense actually triggered (console message)

**Q: Router not changing color**
- A: Defense may not have activated (threshold not reached)
- Try with more attackers: `--nAttackers=10`

**Q: Blocked count is zero**
- A: Attack may not be enabled: `--enableAttack=true`
- Or simulation too short: `--simulationTime=30`
