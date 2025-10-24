#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleDDoSSimulation");

int main(int argc, char *argv[])
{
  // Simulation parameters
  uint32_t nAttackers = 5;
  uint32_t nLegitimate = 3;
  double simulationTime = 20.0;
  bool enableAttack = true;
  
  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("nAttackers", "Number of attacking nodes", nAttackers);
  cmd.AddValue("nLegitimate", "Number of legitimate clients", nLegitimate);
  cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue("enableAttack", "Enable DDoS attack", enableAttack);
  cmd.Parse(argc, argv);

  std::cout << "=== DDoS Simulation with NetAnim ===" << std::endl;
  std::cout << "Attackers: " << nAttackers << ", Legitimate: " << nLegitimate << std::endl;
  std::cout << "Attack: " << (enableAttack ? "ENABLED" : "DISABLED") << std::endl;

  // Create nodes
  NodeContainer attackers, legitimateClients, server, router;
  attackers.Create(nAttackers);
  legitimateClients.Create(nLegitimate);
  server.Create(1);
  router.Create(1);

  // Create topology
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  NetDeviceContainer devices;
  
  // Connect nodes to router
  for (uint32_t i = 0; i < nAttackers; ++i)
    devices.Add(p2p.Install(attackers.Get(i), router.Get(0)));
  for (uint32_t i = 0; i < nLegitimate; ++i)
    devices.Add(p2p.Install(legitimateClients.Get(i), router.Get(0)));

  // Server connection (bottleneck)
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  devices.Add(p2p.Install(router.Get(0), server.Get(0)));

  // Install Internet stack
  InternetStackHelper stack;
  stack.InstallAll();

  // Assign IP addresses
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  address.Assign(devices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Setup applications
  uint16_t port = 9;
  Ipv4Address serverIP = server.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

  // Server
  UdpEchoServerHelper echoServer(port);
  ApplicationContainer serverApps = echoServer.Install(server);
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(simulationTime));

  // Legitimate clients (low rate)
  UdpEchoClientHelper legit(serverIP, port);
  legit.SetAttribute("MaxPackets", UintegerValue(50));
  legit.SetAttribute("Interval", TimeValue(Seconds(0.2))); // 5 pps
  legit.SetAttribute("PacketSize", UintegerValue(256));

  ApplicationContainer legitApps = legit.Install(legitimateClients);
  legitApps.Start(Seconds(2.0));
  legitApps.Stop(Seconds(simulationTime));

  // Attack traffic (high rate)
  if (enableAttack)
  {
    UdpEchoClientHelper attack(serverIP, port);
    attack.SetAttribute("MaxPackets", UintegerValue(1000));
    attack.SetAttribute("Interval", TimeValue(Seconds(0.001))); // 1000 pps
    attack.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer attackApps = attack.Install(attackers);
    attackApps.Start(Seconds(5.0));
    attackApps.Stop(Seconds(15.0));
  }

  // NetAnim setup
  AnimationInterface anim("ddos-simulation.xml");

  // Color nodes for visualization
  for (uint32_t i = 0; i < nAttackers; ++i)
    anim.UpdateNodeColor(attackers.Get(i), 255, 0, 0); // Red for attackers
  for (uint32_t i = 0; i < nLegitimate; ++i)
    anim.UpdateNodeColor(legitimateClients.Get(i), 0, 255, 0); // Green for legitimate
  anim.UpdateNodeColor(router.Get(0), 0, 0, 255); // Blue for router
  anim.UpdateNodeColor(server.Get(0), 255, 165, 0); // Orange for server

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

  // Simple statistics
  monitor->CheckForLostPackets();
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  std::cout << "\n=== Results ===" << std::endl;
  std::cout << "Total flows: " << stats.size() << std::endl;

  uint32_t totalTx = 0, totalRx = 0;
  for (auto &flow : stats)
  {
    totalTx += flow.second.txPackets;
    totalRx += flow.second.rxPackets;
  }

  std::cout << "Packets sent: " << totalTx << std::endl;
  std::cout << "Packets received: " << totalRx << std::endl;
  if (totalTx > 0)
    std::cout << "Success rate: " << (double)totalRx / totalTx * 100 << "%" << std::endl;

  Simulator::Destroy();

  std::cout << "\n=== Files Generated ===" << std::endl;
  std::cout << "- ddos-simulation.xml (NetAnim animation file)" << std::endl;

  return 0;
}
