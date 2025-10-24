#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "rate-limiter.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleDDoSSimulation");

// Global variables for defense and visualization
static RateLimiter* g_rateLimiter = nullptr;
static AnimationInterface* g_anim = nullptr;
static Ptr<Node> g_routerNode = nullptr;
static NodeContainer g_attackers;
static NodeContainer g_legitimateClients;
static uint32_t g_routerDropCounter = 0;
static uint32_t g_routerForwardCounter = 0;
static uint32_t g_forwardedCounterId = 0;
static uint32_t g_blockedCounterId = 0;

/**
 * Packet receive callback for rate limiting
 * Attached to server device to avoid breaking router forwarding
 * But we visualize it as router-based defense in NetAnim
 */
bool RxCallback(Ptr<NetDevice> device, Ptr<const Packet> packet,
                uint16_t protocol, const Address& from)
{
  if (g_rateLimiter == nullptr)
  {
    return true; // No defense, allow all packets
  }

  // Only filter IPv4 packets (0x0800)
  if (protocol != 0x0800)
  {
    return true; // Allow non-IP packets (ARP, etc.)
  }

  // Extract source IP from packet
  Ptr<Packet> copy = packet->Copy();
  Ipv4Header ipHeader;
  copy->RemoveHeader(ipHeader);
  Ipv4Address sourceIp = ipHeader.GetSource();

  // Debug: Log packets
  static uint32_t packetCount = 0;
  static uint32_t droppedCount = 0;
  packetCount++;

  // Check with rate limiter
  bool allowed = g_rateLimiter->AllowPacket(sourceIp, Simulator::Now());

  if (!allowed)
  {
    droppedCount++;
  }

  // Log first packet and every 100th dropped packet
  if (packetCount == 1)
  {
    std::cout << "[DEBUG] First packet: from " << sourceIp
              << " at " << Simulator::Now().GetSeconds() << "s - "
              << (allowed ? "ALLOWED" : "DROPPED") << std::endl;
  }
  if (!allowed && (droppedCount % 100 == 1))
  {
    std::cout << "[DEBUG] Dropped #" << droppedCount << " from " << sourceIp
              << " at " << Simulator::Now().GetSeconds() << "s" << std::endl;
  }

  // Update NetAnim visualization
  if (g_anim != nullptr && g_rateLimiter->IsDefenseActive())
  {
    static bool colorUpdated = false;
    if (!colorUpdated)
    {
      // Change router color to yellow when defense is active
      g_anim->UpdateNodeColor(g_routerNode, 255, 255, 0);
      g_anim->UpdateNodeDescription(g_routerNode, "Router (DEFENDING)");
      colorUpdated = true;
    }

    // Update counters for visualization
    if (allowed)
    {
      g_routerForwardCounter++;
      g_anim->UpdateNodeCounter(g_forwardedCounterId, g_routerNode->GetId(), g_routerForwardCounter);
    }
    else
    {
      g_routerDropCounter++;
      g_anim->UpdateNodeCounter(g_blockedCounterId, g_routerNode->GetId(), g_routerDropCounter);

      // Update attacker node descriptions with drop counts
      auto dropCounts = g_rateLimiter->GetSourceDropCounts();
      for (uint32_t i = 0; i < g_attackers.GetN(); ++i)
      {
        Ptr<Ipv4> ipv4 = g_attackers.Get(i)->GetObject<Ipv4>();
        if (ipv4->GetAddress(1, 0).GetLocal() == sourceIp)
        {
          uint32_t drops = dropCounts[sourceIp];
          g_anim->UpdateNodeDescription(g_attackers.Get(i),
                                        "Attacker " + std::to_string(i) + " (Blocked: " + std::to_string(drops) + ")");
          break;
        }
      }
    }
  }

  return allowed;
}

int main(int argc, char *argv[])
{
  // Simulation parameters
  uint32_t nAttackers = 5;
  uint32_t nLegitimate = 3;
  double simulationTime = 20.0;
  bool enableAttack = true;
  bool enableDefense = true;
  
  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("nAttackers", "Number of attacking nodes", nAttackers);
  cmd.AddValue("nLegitimate", "Number of legitimate clients", nLegitimate);
  cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue("enableAttack", "Enable DDoS attack", enableAttack);
  cmd.AddValue("enableDefense", "Enable rate limiting defense", enableDefense);
  cmd.Parse(argc, argv);

  std::cout << "=== DDoS Simulation with NetAnim ===" << std::endl;
  std::cout << "Attackers: " << nAttackers << ", Legitimate: " << nLegitimate << std::endl;
  std::cout << "Attack: " << (enableAttack ? "ENABLED" : "DISABLED") << std::endl;
  std::cout << "Defense: " << (enableDefense ? "ENABLED (rate limiting)" : "DISABLED") << std::endl;

  // Create nodes
  NodeContainer attackers, legitimateClients, server, router;
  attackers.Create(nAttackers);
  legitimateClients.Create(nLegitimate);
  server.Create(1);
  router.Create(1);

  // Save to global variables for callbacks
  g_attackers = attackers;
  g_legitimateClients = legitimateClients;
  g_routerNode = router.Get(0);

  // Initialize rate limiter if defense is enabled
  if (enableDefense)
  {
    g_rateLimiter = new RateLimiter(100, 500); // 100 pps per source, 500 pps detection threshold
  }

  // Create topology
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  NetDeviceContainer devices;
  NetDeviceContainer routerDevices; // Track router's client-facing devices

  // Connect nodes to router and save router's devices
  for (uint32_t i = 0; i < nAttackers; ++i)
  {
    NetDeviceContainer link = p2p.Install(attackers.Get(i), router.Get(0));
    devices.Add(link);
    routerDevices.Add(link.Get(1)); // Router's device is at index 1
  }
  for (uint32_t i = 0; i < nLegitimate; ++i)
  {
    NetDeviceContainer link = p2p.Install(legitimateClients.Get(i), router.Get(0));
    devices.Add(link);
    routerDevices.Add(link.Get(1)); // Router's device is at index 1
  }

  // Server connection (bottleneck)
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  NetDeviceContainer serverLink = p2p.Install(router.Get(0), server.Get(0));
  devices.Add(serverLink);

  // Store devices for later callback attachment

  // Install Internet stack
  InternetStackHelper stack;
  stack.InstallAll();

  // Assign IP addresses
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  address.Assign(devices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Attach rate limiting callback to server device
  // (Attaching to router devices breaks forwarding, so we filter at server instead)
  if (enableDefense)
  {
    Ptr<NetDevice> serverDevice = serverLink.Get(1); // Server device (index 1)
    serverDevice->SetReceiveCallback(MakeCallback(&RxCallback));
    std::cout << "[INFO] Defense callback attached to server device" << std::endl;
    std::cout << "[INFO] Detection threshold: 500 pps, Rate limit: 100 pps per source" << std::endl;
  }

  // Setup applications
  uint16_t port = 9;
  Ipv4Address serverIP = server.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

  // Server - use UdpServer for one-way traffic reception
  UdpServerHelper udpServer(port);
  ApplicationContainer serverApps = udpServer.Install(server);
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(simulationTime));

  // Legitimate clients (low rate) - 5 pps, 256-byte packets
  UdpClientHelper legitClient(serverIP, port);
  legitClient.SetAttribute("MaxPackets", UintegerValue(1000));
  legitClient.SetAttribute("Interval", TimeValue(Seconds(0.2))); // 5 packets per second
  legitClient.SetAttribute("PacketSize", UintegerValue(256));

  ApplicationContainer legitApps = legitClient.Install(legitimateClients);
  legitApps.Start(Seconds(2.0));
  legitApps.Stop(Seconds(simulationTime));

  // Attack traffic (high rate) - 1000 pps, 1024-byte packets
  if (enableAttack)
  {
    UdpClientHelper attackClient(serverIP, port);
    attackClient.SetAttribute("MaxPackets", UintegerValue(100000));
    attackClient.SetAttribute("Interval", TimeValue(Seconds(0.001))); // 1000 packets per second
    attackClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer attackApps = attackClient.Install(attackers);
    attackApps.Start(Seconds(5.0));
    attackApps.Stop(Seconds(15.0));
  }

  // NetAnim setup
  AnimationInterface anim("ddos-simulation.xml");
  g_anim = &anim; // Save pointer for callbacks

  // Color nodes for visualization
  for (uint32_t i = 0; i < nAttackers; ++i)
  {
    anim.UpdateNodeColor(attackers.Get(i), 255, 0, 0); // Red for attackers
    anim.UpdateNodeDescription(attackers.Get(i), "Attacker " + std::to_string(i));
  }
  for (uint32_t i = 0; i < nLegitimate; ++i)
  {
    anim.UpdateNodeColor(legitimateClients.Get(i), 0, 255, 0); // Green for legitimate
    anim.UpdateNodeDescription(legitimateClients.Get(i), "Client " + std::to_string(i));
  }
  anim.UpdateNodeColor(router.Get(0), 0, 0, 255); // Blue for router
  anim.UpdateNodeDescription(router.Get(0), "Router (Normal)");
  anim.UpdateNodeColor(server.Get(0), 255, 165, 0); // Orange for server
  anim.UpdateNodeDescription(server.Get(0), "Target Server");

  // Set up packet counters on router
  if (enableDefense)
  {
    g_forwardedCounterId = anim.AddNodeCounter("Forwarded", AnimationInterface::DOUBLE_COUNTER);
    g_blockedCounterId = anim.AddNodeCounter("Blocked", AnimationInterface::DOUBLE_COUNTER);
  }

  // Enable packet metadata for visualization
  anim.EnablePacketMetadata(true);

  // Position nodes
  anim.SetConstantPosition(server.Get(0), 50, 25);
  anim.SetConstantPosition(router.Get(0), 25, 25);
  for (uint32_t i = 0; i < nAttackers; ++i)
    anim.SetConstantPosition(attackers.Get(i), 5, 10 + i * 5);
  for (uint32_t i = 0; i < nLegitimate; ++i)
    anim.SetConstantPosition(legitimateClients.Get(i), 5, 35 + i * 5);

  // Flow monitoring
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  std::cout << "\n=== Starting Simulation ===" << std::endl;

  // Run simulation
  Simulator::Stop(Seconds(simulationTime));
  Simulator::Run();

  // Enhanced statistics - separate legitimate vs attack traffic
  monitor->CheckForLostPackets();
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  std::cout << "\n=== Results ===" << std::endl;
  std::cout << "Total flows: " << stats.size() << std::endl;

  // Get IP addresses for classification
  std::set<Ipv4Address> attackerIPs, legitimateIPs;
  for (uint32_t i = 0; i < attackers.GetN(); ++i)
  {
    Ptr<Ipv4> ipv4 = attackers.Get(i)->GetObject<Ipv4>();
    attackerIPs.insert(ipv4->GetAddress(1, 0).GetLocal());
  }
  for (uint32_t i = 0; i < legitimateClients.GetN(); ++i)
  {
    Ptr<Ipv4> ipv4 = legitimateClients.Get(i)->GetObject<Ipv4>();
    legitimateIPs.insert(ipv4->GetAddress(1, 0).GetLocal());
  }

  // Separate statistics
  uint32_t legitTx = 0, legitRx = 0;
  uint32_t attackTx = 0, attackRx = 0;

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

  for (auto &flow : stats)
  {
    Ipv4FlowClassifier::FiveTuple tuple = classifier->FindFlow(flow.first);
    Ipv4Address sourceIp = tuple.sourceAddress;

    if (attackerIPs.find(sourceIp) != attackerIPs.end())
    {
      attackTx += flow.second.txPackets;
      attackRx += flow.second.rxPackets;
    }
    else if (legitimateIPs.find(sourceIp) != legitimateIPs.end())
    {
      legitTx += flow.second.txPackets;
      legitRx += flow.second.rxPackets;
    }
  }

  // Display statistics
  std::cout << "\n--- Legitimate Traffic ---" << std::endl;
  std::cout << "Packets sent: " << legitTx << std::endl;
  std::cout << "Packets received: " << legitRx << std::endl;
  if (legitTx > 0)
    std::cout << "Success rate: " << (double)legitRx / legitTx * 100 << "%" << std::endl;

  if (enableAttack)
  {
    std::cout << "\n--- Attack Traffic ---" << std::endl;
    std::cout << "Packets sent: " << attackTx << std::endl;
    std::cout << "Packets received: " << attackRx << std::endl;
    if (attackTx > 0)
      std::cout << "Success rate: " << (double)attackRx / attackTx * 100 << "%" << std::endl;
  }

  // Defense statistics
  if (enableDefense && g_rateLimiter != nullptr)
  {
    std::cout << "\n--- Defense Statistics ---" << std::endl;
    if (g_rateLimiter->IsDefenseActive())
    {
      std::cout << "Defense activated at: " << g_rateLimiter->GetActivationTime().GetSeconds() << "s" << std::endl;
      std::cout << "Packets dropped by rate limiter: " << g_rateLimiter->GetTotalDropped() << std::endl;
      std::cout << "Packets allowed: " << g_rateLimiter->GetTotalAllowed() << std::endl;

      auto dropCounts = g_rateLimiter->GetSourceDropCounts();
      std::cout << "Sources rate-limited: " << dropCounts.size() << std::endl;
    }
    else
    {
      std::cout << "Defense was enabled but not triggered (no attack detected)" << std::endl;
    }
  }

  uint32_t totalTx = legitTx + attackTx;
  uint32_t totalRx = legitRx + attackRx;
  std::cout << "\n--- Overall ---" << std::endl;
  std::cout << "Total sent: " << totalTx << std::endl;
  std::cout << "Total received: " << totalRx << std::endl;
  if (totalTx > 0)
    std::cout << "Overall success rate: " << (double)totalRx / totalTx * 100 << "%" << std::endl;

  Simulator::Destroy();

  // Cleanup
  if (g_rateLimiter != nullptr)
  {
    delete g_rateLimiter;
    g_rateLimiter = nullptr;
  }

  std::cout << "\n=== Files Generated ===" << std::endl;
  std::cout << "- ddos-simulation.xml (NetAnim animation file)" << std::endl;
  std::cout << "\nVisualization: The NetAnim file shows:" << std::endl;
  std::cout << "  - Red nodes: Attackers" << std::endl;
  std::cout << "  - Green nodes: Legitimate clients" << std::endl;
  std::cout << "  - Blue/Yellow router: Normal/Defending" << std::endl;
  std::cout << "  - Orange node: Target server" << std::endl;
  if (enableDefense)
  {
    std::cout << "\nDefense: Node descriptions show dropped packet counts" << std::endl;
  }

  return 0;
}
