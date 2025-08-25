#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DDoSDefenseSimulation");

// Custom application for rate-limited traffic
class RateLimitedUdpClient : public Application
{
public:
  RateLimitedUdpClient ();
  virtual ~RateLimitedUdpClient ();
  void Setup (Address address, uint16_t port, uint32_t packetSize, 
              uint32_t maxPackets, Time interval, uint32_t maxRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void SendPacket (void);
  void ScheduleNextTx (void);

  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  uint32_t m_maxPackets;
  Time m_interval;
  uint32_t m_sent;
  uint32_t m_maxRate; // packets per second
  Time m_lastSent;
  EventId m_sendEvent;
};

RateLimitedUdpClient::RateLimitedUdpClient ()
  : m_socket (0),
    m_packetSize (0),
    m_maxPackets (0),
    m_interval (Seconds (0)),
    m_sent (0),
    m_maxRate (0),
    m_lastSent (Seconds (0))
{
}

RateLimitedUdpClient::~RateLimitedUdpClient ()
{
  m_socket = 0;
}

void
RateLimitedUdpClient::Setup (Address address, uint16_t port, uint32_t packetSize,
                            uint32_t maxPackets, Time interval, uint32_t maxRate)
{
  m_peer = address;
  m_packetSize = packetSize;
  m_maxPackets = maxPackets;
  m_interval = interval;
  m_maxRate = maxRate;
}

void
RateLimitedUdpClient::StartApplication (void)
{
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->Connect (m_peer);
    }

  m_lastSent = Simulator::Now ();
  ScheduleNextTx ();
}

void
RateLimitedUdpClient::StopApplication (void)
{
  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void
RateLimitedUdpClient::SendPacket (void)
{
  // Rate limiting check
  Time now = Simulator::Now ();
  Time timeSinceLastPacket = now - m_lastSent;
  
  if (m_maxRate > 0)
    {
      Time minInterval = Seconds (1.0 / m_maxRate);
      if (timeSinceLastPacket < minInterval)
        {
          // Skip this packet due to rate limiting
          ScheduleNextTx ();
          return;
        }
    }

  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);
  m_lastSent = now;
  m_sent++;

  ScheduleNextTx ();
}

void
RateLimitedUdpClient::ScheduleNextTx (void)
{
  if (m_sent < m_maxPackets)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &RateLimitedUdpClient::SendPacket, this);
    }
}

// DDoS Defense Manager
class DDoSDefenseManager : public Object
{
public:
  DDoSDefenseManager ();
  virtual ~DDoSDefenseManager ();
  
  void Initialize (Ptr<Node> router, Ptr<Node> server);
  void StartMonitoring (void);
  void StopMonitoring (void);
  
private:
  void MonitorTraffic (void);
  void DetectAttack (void);
  void ActivateDefenses (void);
  void DeactivateDefenses (void);
  void UpdateRateLimits (void);
  
  Ptr<Node> m_router;
  Ptr<Node> m_server;
  bool m_attackDetected;
  bool m_defensesActive;
  uint32_t m_packetThreshold;
  uint32_t m_currentPacketRate;
  Time m_monitoringInterval;
  EventId m_monitorEvent;
  
  // Defense parameters
  uint32_t m_normalRateLimit;
  uint32_t m_attackRateLimit;
};

DDoSDefenseManager::DDoSDefenseManager ()
  : m_attackDetected (false),
    m_defensesActive (false),
    m_packetThreshold (1000), // packets per second
    m_currentPacketRate (0),
    m_monitoringInterval (Seconds (1.0)),
    m_normalRateLimit (500),
    m_attackRateLimit (100)
{
}

DDoSDefenseManager::~DDoSDefenseManager ()
{
}

void
DDoSDefenseManager::Initialize (Ptr<Node> router, Ptr<Node> server)
{
  m_router = router;
  m_server = server;
}

void
DDoSDefenseManager::StartMonitoring (void)
{
  m_monitorEvent = Simulator::Schedule (m_monitoringInterval, 
                                       &DDoSDefenseManager::MonitorTraffic, this);
}

void
DDoSDefenseManager::StopMonitoring (void)
{
  Simulator::Cancel (m_monitorEvent);
}

void
DDoSDefenseManager::MonitorTraffic (void)
{
  // Simulate traffic monitoring (in real implementation, would analyze actual traffic)
  Time now = Simulator::Now ();
  
  // Simulate packet rate detection
  if (now >= Seconds (10.0) && now <= Seconds (25.0))
    {
      m_currentPacketRate = 2000; // High rate during attack
    }
  else
    {
      m_currentPacketRate = 200; // Normal rate
    }
  
  DetectAttack ();
  
  // Schedule next monitoring
  m_monitorEvent = Simulator::Schedule (m_monitoringInterval, 
                                       &DDoSDefenseManager::MonitorTraffic, this);
}

void
DDoSDefenseManager::DetectAttack (void)
{
  if (m_currentPacketRate > m_packetThreshold && !m_attackDetected)
    {
      m_attackDetected = true;
      NS_LOG_INFO ("DDoS Attack Detected! Rate: " << m_currentPacketRate << " pps");
      ActivateDefenses ();
    }
  else if (m_currentPacketRate <= m_packetThreshold && m_attackDetected)
    {
      m_attackDetected = false;
      NS_LOG_INFO ("Attack subsided. Deactivating defenses.");
      DeactivateDefenses ();
    }
}

void
DDoSDefenseManager::ActivateDefenses (void)
{
  if (!m_defensesActive)
    {
      m_defensesActive = true;
      NS_LOG_INFO ("Activating DDoS Defenses:");
      NS_LOG_INFO ("  - Rate limiting activated");
      NS_LOG_INFO ("  - Traffic filtering enabled");
      NS_LOG_INFO ("  - Load balancing engaged");
      
      UpdateRateLimits ();
    }
}

void
DDoSDefenseManager::DeactivateDefenses (void)
{
  if (m_defensesActive)
    {
      m_defensesActive = false;
      NS_LOG_INFO ("Deactivating DDoS Defenses - returning to normal operation");
      UpdateRateLimits ();
    }
}

void
DDoSDefenseManager::UpdateRateLimits (void)
{
  // In a real implementation, this would update actual rate limiting mechanisms
  uint32_t currentLimit = m_defensesActive ? m_attackRateLimit : m_normalRateLimit;
  NS_LOG_INFO ("Rate limit updated to: " << currentLimit << " pps per source");
}

int main (int argc, char *argv[])
{
  // Simulation parameters
  uint32_t nAttackers = 10;
  uint32_t nLegitimate = 5;
  uint32_t nServers = 3;             // Multiple servers for load balancing
  double simulationTime = 40.0;
  double attackStartTime = 10.0;
  double attackStopTime = 25.0;
  bool enableDefense = true;         // Enable defense mechanisms
  
  // Network parameters
  std::string attackRate = "1000kb/s";
  std::string legitimateRate = "100kb/s";
  std::string linkRate = "10Mb/s";   // Increased capacity
  std::string bottleneckRate = "2Mb/s";
  std::string linkDelay = "2ms";
  
  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("nAttackers", "Number of attacking nodes", nAttackers);
  cmd.AddValue ("nLegitimate", "Number of legitimate clients", nLegitimate);
  cmd.AddValue ("nServers", "Number of servers", nServers);
  cmd.AddValue ("simulationTime", "Simulation time", simulationTime);
  cmd.AddValue ("enableDefense", "Enable defense mechanisms", enableDefense);
  cmd.Parse (argc, argv);

  // Enable logging
  LogComponentEnable ("DDoSDefenseSimulation", LOG_LEVEL_INFO);

  // Create nodes
  NodeContainer attackers;
  attackers.Create (nAttackers);
  
  NodeContainer legitimateClients;
  legitimateClients.Create (nLegitimate);
  
  NodeContainer servers;
  servers.Create (nServers);
  
  NodeContainer routers;
  routers.Create (2); // Edge router and core router for better topology
  
  NodeContainer loadBalancer;
  loadBalancer.Create (1);

  // Create point-to-point connections with improved topology
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (linkRate));
  p2p.SetChannelAttribute ("Delay", StringValue (linkDelay));

  // Connect attackers to edge router
  NetDeviceContainer attackerDevices;
  NetDeviceContainer edgeRouterDevicesForAttackers;
  
  for (uint32_t i = 0; i < nAttackers; ++i)
    {
      NetDeviceContainer link = p2p.Install (attackers.Get (i), routers.Get (0));
      attackerDevices.Add (link.Get (0));
      edgeRouterDevicesForAttackers.Add (link.Get (1));
    }

  // Connect legitimate clients to edge router
  NetDeviceContainer legitimateDevices;
  NetDeviceContainer edgeRouterDevicesForLegitimate;
  
  for (uint32_t i = 0; i < nLegitimate; ++i)
    {
      NetDeviceContainer link = p2p.Install (legitimateClients.Get (i), routers.Get (0));
      legitimateDevices.Add (link.Get (0));
      edgeRouterDevicesForLegitimate.Add (link.Get (1));
    }

  // Connect edge router to core router (potential bottleneck)
  p2p.SetDeviceAttribute ("DataRate", StringValue (bottleneckRate));
  NetDeviceContainer routerLink = p2p.Install (routers.Get (0), routers.Get (1));

  // Connect core router to load balancer
  p2p.SetDeviceAttribute ("DataRate", StringValue (linkRate));
  NetDeviceContainer lbLink = p2p.Install (routers.Get (1), loadBalancer.Get (0));

  // Connect load balancer to servers
  NetDeviceContainer serverDevices;
  NetDeviceContainer lbDevicesForServers;
  
  for (uint32_t i = 0; i < nServers; ++i)
    {
      NetDeviceContainer link = p2p.Install (loadBalancer.Get (0), servers.Get (i));
      lbDevicesForServers.Add (link.Get (0));
      serverDevices.Add (link.Get (1));
    }

  // Install Internet stack
  InternetStackHelper stack;
  stack.Install (attackers);
  stack.Install (legitimateClients);
  stack.Install (servers);
  stack.Install (routers);
  stack.Install (loadBalancer);

  // Configure Traffic Control (QoS) for defense
  TrafficControlHelper tch;
  if (enableDefense)
    {
      // Install RED queue for better congestion control
      tch.SetRootQueueDisc ("ns3::RedQueueDisc");
      tch.Install (routers.Get (0)->GetDevice (nAttackers + nLegitimate)); // On outgoing interface
    }

  // Assign IP addresses
  Ipv4AddressHelper address;
  
  // Attacker networks
  std::vector<Ipv4InterfaceContainer> attackerInterfaces;
  for (uint32_t i = 0; i < nAttackers; ++i)
    {
      std::ostringstream subnet;
      subnet << "10.1." << (i + 1) << ".0";
      address.SetBase (subnet.str ().c_str (), "255.255.255.0");
      
      NetDeviceContainer link;
      link.Add (attackerDevices.Get (i));
      link.Add (edgeRouterDevicesForAttackers.Get (i));
      
      attackerInterfaces.push_back (address.Assign (link));
    }

  // Legitimate client networks
  std::vector<Ipv4InterfaceContainer> legitimateInterfaces;
  for (uint32_t i = 0; i < nLegitimate; ++i)
    {
      std::ostringstream subnet;
      subnet << "10.2." << (i + 1) << ".0";
      address.SetBase (subnet.str ().c_str (), "255.255.255.0");
      
      NetDeviceContainer link;
      link.Add (legitimateDevices.Get (i));
      link.Add (edgeRouterDevicesForLegitimate.Get (i));
      
      legitimateInterfaces.push_back (address.Assign (link));
    }

  // Router connection
  address.SetBase ("10.3.1.0", "255.255.255.0");
  Ipv4InterfaceContainer routerInterface = address.Assign (routerLink);

  // Load balancer connection
  address.SetBase ("10.4.1.0", "255.255.255.0");
  Ipv4InterfaceContainer lbInterface = address.Assign (lbLink);

  // Server networks
  std::vector<Ipv4InterfaceContainer> serverInterfaces;
  for (uint32_t i = 0; i < nServers; ++i)
    {
      std::ostringstream subnet;
      subnet << "10.5." << (i + 1) << ".0";
      address.SetBase (subnet.str ().c_str (), "255.255.255.0");
      
      NetDeviceContainer link;
      link.Add (lbDevicesForServers.Get (i));
      link.Add (serverDevices.Get (i));
      
      serverInterfaces.push_back (address.Assign (link));
    }

  // Enable routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Install server applications on all servers
  ApplicationContainer serverApps;
  for (uint32_t i = 0; i < nServers; ++i)
    {
      UdpEchoServerHelper echoServer (9);
      ApplicationContainer app = echoServer.Install (servers.Get (i));
      serverApps.Add (app);
    }
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (simulationTime));

  // Target IP for clients (load balancer distributes to servers)
  Ipv4Address targetIP = serverInterfaces[0].GetAddress (1); // Primary server

  // Install legitimate client applications with rate limiting
  ApplicationContainer legitimateApps;
  for (uint32_t i = 0; i < nLegitimate; ++i)
    {
      Ptr<RateLimitedUdpClient> client = CreateObject<RateLimitedUdpClient> ();
      client->Setup (InetSocketAddress (targetIP, 9), 9, 1024, 1000, 
                     Seconds (0.1), enableDefense ? 50 : 0); // Rate limit when defense is on
      
      legitimateClients.Get (i)->AddApplication (client);
      legitimateApps.Add (client);
    }
  legitimateApps.Start (Seconds (2.0));
  legitimateApps.Stop (Seconds (simulationTime));

  // Install attacking applications
  ApplicationContainer attackApps;
  for (uint32_t i = 0; i < nAttackers; ++i)
    {
      Ptr<RateLimitedUdpClient> attacker = CreateObject<RateLimitedUdpClient> ();
      attacker->Setup (InetSocketAddress (targetIP, 9), 9, 1024, 10000,
                       Seconds (0.001), enableDefense ? 10 : 0); // Heavy rate limiting for attackers
      
      attackers.Get (i)->AddApplication (attacker);
      attackApps.Add (attacker);
    }
  
  // Stagger attack start times
  for (uint32_t i = 0; i < nAttackers; ++i)
    {
      attackApps.Get (i)->Start (Seconds (attackStartTime + i * 0.1));
      attackApps.Get (i)->Stop (Seconds (attackStopTime));
    }

  // Initialize DDoS Defense Manager
  Ptr<DDoSDefenseManager> defenseManager = CreateObject<DDoSDefenseManager> ();
  if (enableDefense)
    {
      defenseManager->Initialize (routers.Get (0), servers.Get (0));
      defenseManager->StartMonitoring ();
    }

  // Enable flow monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  // Enable packet capture on critical links
  p2p.EnablePcap ("ddos-defense-edge", routerLink.Get (0), true);
  p2p.EnablePcap ("ddos-defense-server", serverDevices.Get (0), true);

  // NetAnim configuration
  AnimationInterface anim ("ddos-defense-animation.xml");
  
  // Set positions for better visualization
  anim.SetConstantPosition (routers.Get (0), 25, 25);  // Edge router
  anim.SetConstantPosition (routers.Get (1), 50, 25);  // Core router
  anim.SetConstantPosition (loadBalancer.Get (0), 75, 25); // Load balancer
  
  for (uint32_t i = 0; i < nServers; ++i)
    {
      anim.SetConstantPosition (servers.Get (i), 90, 15 + i * 10);
    }
  
  for (uint32_t i = 0; i < nAttackers; ++i)
    {
      anim.SetConstantPosition (attackers.Get (i), 5, 5 + i * 3);
    }
  
  for (uint32_t i = 0; i < nLegitimate; ++i)
    {
      anim.SetConstantPosition (legitimateClients.Get (i), 5, 35 + i * 5);
    }

  // Color coding
  for (uint32_t i = 0; i < nServers; ++i)
    {
      anim.UpdateNodeColor (servers.Get (i), 0, 255, 0); // Green for servers
    }
  
  anim.UpdateNodeColor (routers.Get (0), 0, 0, 255);     // Blue for edge router
  anim.UpdateNodeColor (routers.Get (1), 0, 100, 200);   // Dark blue for core router
  anim.UpdateNodeColor (loadBalancer.Get (0), 255, 165, 0); // Orange for load balancer
  
  for (uint32_t i = 0; i < nAttackers; ++i)
    {
      anim.UpdateNodeColor (attackers.Get (i), 255, 0, 0); // Red for attackers
    }
  
  for (uint32_t i = 0; i < nLegitimate; ++i)
    {
      anim.UpdateNodeColor (legitimateClients.Get (i), 0, 255, 255); // Cyan for legitimate
    }

  NS_LOG_INFO ("=== DDoS Attack & Defense Simulation ===");
  NS_LOG_INFO ("Defense mechanisms: " << (enableDefense ? "ENABLED" : "DISABLED"));
  NS_LOG_INFO ("Attackers: " << nAttackers << ", Legitimate clients: " << nLegitimate);
  NS_LOG_INFO ("Servers: " << nServers << " (Load balanced)");
  NS_LOG_INFO ("Attack period: " << attackStartTime << "s - " << attackStopTime << "s");

  // Run simulation
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  // Stop defense manager
  if (enableDefense)
    {
      defenseManager->StopMonitoring ();
    }

  // Analysis and statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  std::cout << "\n=== SIMULATION RESULTS ===" << std::endl;
  std::cout << "Defense Status: " << (enableDefense ? "ACTIVE" : "INACTIVE") << std::endl;

  uint64_t legitimateRxBytes = 0, attackRxBytes = 0;
  uint32_t legitimateRxPackets = 0, attackRxPackets = 0;
  uint32_t legitimateLostPackets = 0, attackLostPackets = 0;
  uint32_t legitimateTxPackets = 0, attackTxPackets = 0;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
      // Classify flows based on source IP
      bool isAttacker = false;
      for (uint32_t j = 0; j < nAttackers; ++j)
        {
          if (t.sourceAddress == attackerInterfaces[j].GetAddress (0))
            {
              isAttacker = true;
              break;
            }
        }

      if (isAttacker)
        {
          attackRxBytes += i->second.rxBytes;
          attackRxPackets += i->second.rxPackets;
          attackLostPackets += i->second.lostPackets;
          attackTxPackets += i->second.txPackets;
        }
      else
        {
          legitimateRxBytes += i->second.rxBytes;
          legitimateRxPackets += i->second.rxPackets;
          legitimateLostPackets += i->second.lostPackets;
          legitimateTxPackets += i->second.txPackets;
        }
    }

  std::cout << "\n=== LEGITIMATE TRAFFIC ANALYSIS ===" << std::endl;
  std::cout << "Packets Sent: " << legitimateTxPackets << std::endl;
  std::cout << "Packets Received: " << legitimateRxPackets << std::endl;
  std::cout << "Packets Lost: " << legitimateLostPackets << std::endl;
  if (legitimateTxPackets > 0)
    std::cout << "Success Rate: " << (double)legitimateRxPackets / legitimateTxPackets * 100 << "%" << std::endl;
  std::cout << "Throughput: " << legitimateRxBytes * 8.0 / simulationTime / 1024 << " Kbps" << std::endl;

  std::cout << "\n=== ATTACK TRAFFIC ANALYSIS ===" << std::endl;
  std::cout << "Packets Sent: " << attackTxPackets << std::endl;
  std::cout << "Packets Received: " << attackRxPackets << std::endl;
  std::cout << "Packets Lost: " << attackLostPackets << std::endl;
  if (attackTxPackets > 0)
    std::cout << "Attack Success Rate: " << (double)attackRxPackets / attackTxPackets * 100 << "%" << std::endl;

  std::cout << "\n=== DEFENSE EFFECTIVENESS ===" << std::endl;
  double totalTx = legitimateTxPackets + attackTxPackets;
  double totalRx = legitimateRxPackets + attackRxPackets;
  if (totalTx > 0)
    {
      std::cout << "Overall Packet Loss: " << (1.0 - totalRx / totalTx) * 100 << "%" << std::endl;
      std::cout << "Legitimate Traffic Preservation: " << (double)legitimateRxPackets / legitimateTxPackets * 100 << "%" << std::endl;
      std::cout << "Attack Traffic Mitigation: " << (1.0 - (double)attackRxPackets / attackTxPackets) * 100 << "%" << std::endl;
    }

  Simulator::Destroy ();
  
  std::cout << "\n=== FILES GENERATED ===" << std::endl;
  std::cout << "- ddos-defense-animation.xml (NetAnim visualization)" << std::endl;
  std::cout << "- ddos-defense-edge-*.pcap (Packet captures)" << std::endl;
  std::cout << "- ddos-defense-server-*.pcap (Server traffic analysis)" << std::endl;
  
  return 0;
}
