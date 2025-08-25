#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleDDoSSimulation");

int main (int argc, char *argv[])
{
  // Simulation parameters
  uint32_t nAttackers = 10;
  uint32_t nLegitimate = 5;
  double simulationTime = 30.0;
  double attackStartTime = 5.0;
  double attackStopTime = 20.0;
  bool enableAttack = true;          // Main parameter: enable/disable attack
  
  // Network parameters
  std::string linkRate = "10Mb/s";
  std::string bottleneckRate = "2Mb/s";  // Server link bottleneck
  std::string linkDelay = "2ms";
  
  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("nAttackers", "Number of attacking nodes", nAttackers);
  cmd.AddValue ("nLegitimate", "Number of legitimate clients", nLegitimate);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("attackStartTime", "Attack start time", attackStartTime);
  cmd.AddValue ("attackStopTime", "Attack stop time", attackStopTime);
  cmd.AddValue ("enableAttack", "Enable DDoS attack", enableAttack);
  cmd.Parse (argc, argv);

  // Enable logging
  LogComponentEnable ("SimpleDDoSSimulation", LOG_LEVEL_INFO);

  std::cout << "=== Simple DDoS Simulation ===" << std::endl;
  std::cout << "Attack enabled: " << (enableAttack ? "YES" : "NO") << std::endl;
  std::cout << "Attackers: " << nAttackers << std::endl;
  std::cout << "Legitimate clients: " << nLegitimate << std::endl;
  if (enableAttack)
    {
      std::cout << "Attack period: " << attackStartTime << "s - " << attackStopTime << "s" << std::endl;
    }
  std::cout << "Simulation time: " << simulationTime << "s" << std::endl;

  // Create nodes
  NodeContainer attackers;
  attackers.Create (nAttackers);
  
  NodeContainer legitimateClients;
  legitimateClients.Create (nLegitimate);
  
  NodeContainer server;
  server.Create (1);
  
  NodeContainer router;
  router.Create (1);

  // Create network topology
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (linkRate));
  p2p.SetChannelAttribute ("Delay", StringValue (linkDelay));

  NetDeviceContainer devices;
  
  // Connect attackers to router
  for (uint32_t i = 0; i < nAttackers; ++i)
    {
      NetDeviceContainer link = p2p.Install (attackers.Get (i), router.Get (0));
      devices.Add (link);
    }
  
  // Connect legitimate clients to router
  for (uint32_t i = 0; i < nLegitimate; ++i)
    {
      NetDeviceContainer link = p2p.Install (legitimateClients.Get (i), router.Get (0));
      devices.Add (link);
    }
  
  // Connect router to server with bottleneck
  p2p.SetDeviceAttribute ("DataRate", StringValue (bottleneckRate));
  NetDeviceContainer serverLink = p2p.Install (router.Get (0), server.Get (0));
  devices.Add (serverLink);

  // Install Internet stack
  InternetStackHelper stack;
  stack.Install (attackers);
  stack.Install (legitimateClients);
  stack.Install (server);
  stack.Install (router);

  // Assign IP addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Enable routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Install server application
  uint16_t port = 9;
  UdpEchoServerHelper echoServer (port);
  ApplicationContainer serverApps = echoServer.Install (server.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (simulationTime));

  // Get server IP address
  Ipv4Address serverIP = server.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();

  // Install legitimate client applications
  UdpEchoClientHelper legitimateClient (serverIP, port);
  legitimateClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  legitimateClient.SetAttribute ("Interval", TimeValue (Seconds (0.1))); // 10 pps
  legitimateClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer legitimateApps;
  for (uint32_t i = 0; i < nLegitimate; ++i)
    {
      ApplicationContainer app = legitimateClient.Install (legitimateClients.Get (i));
      legitimateApps.Add (app);
    }
  legitimateApps.Start (Seconds (2.0));
  legitimateApps.Stop (Seconds (simulationTime - 1.0));

  // Install attacking applications (only if attack is enabled)
  ApplicationContainer attackApps;
  if (enableAttack)
    {
      std::cout << "Configuring attack applications..." << std::endl;
      
      UdpEchoClientHelper attackClient (serverIP, port);
      attackClient.SetAttribute ("MaxPackets", UintegerValue (10000));
      attackClient.SetAttribute ("Interval", TimeValue (Seconds (0.001))); // 1000 pps per attacker
      attackClient.SetAttribute ("PacketSize", UintegerValue (1024));

      for (uint32_t i = 0; i < nAttackers; ++i)
        {
          ApplicationContainer app = attackClient.Install (attackers.Get (i));
          attackApps.Add (app);
        }
      
      // Set attack timing with slight stagger
      for (uint32_t i = 0; i < nAttackers; ++i)
        {
          attackApps.Get (i)->SetStartTime (Seconds (attackStartTime + i * 0.1));
          attackApps.Get (i)->SetStopTime (Seconds (attackStopTime));
        }
      
      std::cout << "Attack configured: " << nAttackers << " attackers, " 
                << (attackStopTime - attackStartTime) << " second duration" << std::endl;
    }
  else
    {
      std::cout << "Attack disabled - only legitimate traffic will run" << std::endl;
    }

  // Enable flow monitor for statistics
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  // Enable packet capture
  p2p.EnablePcap ("simple-ddos", serverLink.Get (0), true);

  std::cout << "\n=== Starting Simulation ===" << std::endl;

  // Run simulation
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  // Collect and analyze statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  std::cout << "\n=== Simulation Results ===" << std::endl;
  std::cout << "Attack Status: " << (enableAttack ? "ENABLED" : "DISABLED") << std::endl;

  uint64_t legitimateRxBytes = 0, attackRxBytes = 0;
  uint32_t legitimateRxPackets = 0, attackRxPackets = 0;
  uint32_t legitimateTxPackets = 0, attackTxPackets = 0;
  uint32_t legitimateLostPackets = 0, attackLostPackets = 0;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
      // Classify flows based on source IP
      bool isAttacker = false;
      if (enableAttack)
        {
          for (uint32_t j = 0; j < nAttackers; ++j)
            {
              Ipv4Address attackerIP = attackers.Get (j)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
              if (t.sourceAddress == attackerIP)
                {
                  isAttacker = true;
                  break;
                }
            }
        }

      if (isAttacker)
        {
          attackRxBytes += i->second.rxBytes;
          attackRxPackets += i->second.rxPackets;
          attackTxPackets += i->second.txPackets;
          attackLostPackets += i->second.lostPackets;
        }
      else
        {
          legitimateRxBytes += i->second.rxBytes;
          legitimateRxPackets += i->second.rxPackets;
          legitimateTxPackets += i->second.txPackets;
          legitimateLostPackets += i->second.lostPackets;
        }
    }

  std::cout << "\n=== Legitimate Traffic Analysis ===" << std::endl;
  std::cout << "Packets Sent: " << legitimateTxPackets << std::endl;
  std::cout << "Packets Received: " << legitimateRxPackets << std::endl;
  std::cout << "Packets Lost: " << legitimateLostPackets << std::endl;
  if (legitimateTxPackets > 0)
    {
      double successRate = (double)legitimateRxPackets / legitimateTxPackets * 100;
      std::cout << "Success Rate: " << successRate << "%" << std::endl;
    }
  std::cout << "Throughput: " << legitimateRxBytes * 8.0 / simulationTime / 1024 << " Kbps" << std::endl;

  if (enableAttack)
    {
      std::cout << "\n=== Attack Traffic Analysis ===" << std::endl;
      std::cout << "Packets Sent: " << attackTxPackets << std::endl;
      std::cout << "Packets Received: " << attackRxPackets << std::endl;
      std::cout << "Packets Lost: " << attackLostPackets << std::endl;
      if (attackTxPackets > 0)
        {
          double attackSuccessRate = (double)attackRxPackets / attackTxPackets * 100;
          std::cout << "Attack Success Rate: " << attackSuccessRate << "%" << std::endl;
        }

      std::cout << "\n=== Network Impact ===" << std::endl;
      uint32_t totalTxPackets = legitimateTxPackets + attackTxPackets;
      uint32_t totalRxPackets = legitimateRxPackets + attackRxPackets;
      uint32_t totalLostPackets = legitimateLostPackets + attackLostPackets;
      
      if (totalTxPackets > 0)
        {
          std::cout << "Overall Packet Loss: " << (double)totalLostPackets / totalTxPackets * 100 << "%" << std::endl;
        }
      
      std::cout << "Network Congestion Level: ";
      if (totalLostPackets > totalTxPackets * 0.1)
        std::cout << "HIGH" << std::endl;
      else if (totalLostPackets > totalTxPackets * 0.01)
        std::cout << "MEDIUM" << std::endl;
      else
        std::cout << "LOW" << std::endl;
    }

  std::cout << "\n=== Performance Comparison ===" << std::endl;
  if (enableAttack)
    {
      std::cout << "This simulation shows network behavior UNDER ATTACK" << std::endl;
      std::cout << "Run with '--enableAttack=false' to see normal behavior" << std::endl;
    }
  else
    {
      std::cout << "This simulation shows NORMAL network behavior" << std::endl;
      std::cout << "Run with '--enableAttack=true' to see attack impact" << std::endl;
    }

  Simulator::Destroy ();
  
  std::cout << "\n=== Files Generated ===" << std::endl;
  std::cout << "- simple-ddos-*.pcap (Packet captures for analysis)" << std::endl;
  
  return 0;
}