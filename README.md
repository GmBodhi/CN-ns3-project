# DDoS Attack & Defense Simulation in ns-3

A comprehensive network simulation demonstrating DDoS (Distributed Denial of Service) attacks and advanced defense mechanisms using the ns-3 network simulator.

## 🎯 Overview

This simulation models a realistic DDoS attack scenario and implements multiple defense strategies to protect network infrastructure. It's designed for educational purposes, security research, and understanding network defense mechanisms.

### Key Features
- **Multi-vector DDoS Attack**: Simulates coordinated attacks from multiple sources
- **Layered Defense System**: Implements rate limiting, traffic filtering, and load balancing
- **Real-time Attack Detection**: Automatic threat identification and response
- **Comprehensive Analytics**: Detailed performance metrics and visualization
- **Configurable Parameters**: Customizable attack and defense scenarios

## 🛠️ Prerequisites

### System Requirements
- **ns-3.36+** (Network Simulator 3)
- **C++ Compiler** (GCC 7+ or Clang 6+)
- **Python 3.6+** (for ns-3 build system)
- **Git** (for version control)

### Installing ns-3
```bash
# Download ns-3
wget https://www.nsnam.org/releases/ns-allinone-3.39.tar.bz2
tar -xjf ns-allinone-3.39.tar.bz2
cd ns-allinone-3.39

# Build ns-3
./build.py --enable-examples --enable-tests

# Or use the minimal installation
cd ns-3.39
./ns3 configure --build-profile=optimized --enable-examples
./ns3 build
```

## 🚀 Installation & Setup

### 1. Clone or Copy the Simulation Files
```bash
# Navigate to ns-3 scratch directory
cd ns-3.39/scratch/

# Copy the simulation file
cp /path/to/ddos-defense-simulation.cc .
```

### 2. Build the Simulation
```bash
# From ns-3 root directory
./ns3 build
```

### 3. Verify Installation
```bash
# Test basic compilation
./ns3 run ddos-defense-simulation --help
```

## 🎮 Running the Simulation

### Basic Execution
```bash
# Run with default parameters
./ns3 run ddos-defense-simulation

# Run with verbose output
./ns3 run "ddos-defense-simulation --verbose"
```

### Customizable Parameters

| Parameter | Description | Default | Example |
|-----------|-------------|---------|---------|
| `--nAttackers` | Number of attacking nodes | 10 | `--nAttackers=15` |
| `--nLegitimate` | Number of legitimate clients | 5 | `--nLegitimate=8` |
| `--nServers` | Number of backend servers | 3 | `--nServers=5` |
| `--simulationTime` | Total simulation duration (seconds) | 40.0 | `--simulationTime=60.0` |
| `--enableDefense` | Enable/disable defense mechanisms | true | `--enableDefense=false` |

### Example Commands
```bash
# Large-scale attack simulation
./ns3 run "ddos-defense-simulation --nAttackers=20 --nLegitimate=10 --simulationTime=60"

# Compare defense effectiveness
./ns3 run "ddos-defense-simulation --enableDefense=false"  # No defense
./ns3 run "ddos-defense-simulation --enableDefense=true"   # With defense

# Stress test with minimal servers
./ns3 run "ddos-defense-simulation --nAttackers=25 --nServers=1"
```

## 📊 Understanding the Output

### Console Output
The simulation provides real-time information:

```
=== DDoS Attack & Defense Simulation ===
Defense mechanisms: ENABLED
Attackers: 10, Legitimate clients: 5
Servers: 3 (Load balanced)
Attack period: 10s - 25s

DDoS Attack Detected! Rate: 2000 pps
Activating DDoS Defenses:
  - Rate limiting activated
  - Traffic filtering enabled
  - Load balancing engaged
```

### Performance Metrics
```
=== LEGITIMATE TRAFFIC ANALYSIS ===
Packets Sent: 1250
Packets Received: 1089
Success Rate: 87.12%
Throughput: 1423.5 Kbps

=== ATTACK TRAFFIC ANALYSIS ===
Packets Sent: 15000
Packets Received: 1205
Attack Success Rate: 8.03%

=== DEFENSE EFFECTIVENESS ===
Legitimate Traffic Preservation: 87.12%
Attack Traffic Mitigation: 91.97%
```

## 📁 Generated Files

### Simulation Outputs
- **`ddos-defense-animation.xml`**: NetAnim visualization file
- **`ddos-defense-edge-*.pcap`**: Packet captures from edge router
- **`ddos-defense-server-*.pcap`**: Server traffic analysis files

### Analyzing Results

#### 1. NetAnim Visualization
```bash
# Install NetAnim (if not already installed)
cd ns-3.39/netanim
make clean
qmake NetAnim.pro
make

# Open animation
./NetAnim ddos-defense-animation.xml
```

#### 2. Wireshark Analysis
```bash
# Analyze packet captures
wireshark ddos-defense-edge-*.pcap
wireshark ddos-defense-server-*.pcap
```

#### 3. Flow Monitor Data
The simulation automatically generates detailed flow statistics showing:
- Per-flow throughput and latency
- Packet loss rates by source
- Attack vs. legitimate traffic patterns

## 🏗️ Network Topology

```
Attackers (Red)     Legitimate Clients (Cyan)
     |                        |
     |                        |
  Edge Router (Blue) -------- Core Router (Dark Blue)
                                      |
                               Load Balancer (Orange)
                                      |
                              Servers (Green)
                             /     |     \
                        Server1  Server2  Server3
```

### Node Color Coding
- 🔴 **Red**: Attacking nodes
- 🔵 **Blue**: Edge router (first defense line)
- 🔷 **Dark Blue**: Core router
- 🟠 **Orange**: Load balancer
- 🟢 **Green**: Servers
- 🔵 **Cyan**: Legitimate clients

## 🛡️ Defense Mechanisms

### 1. Rate Limiting
- **Normal Operation**: 500 packets/second per source
- **Under Attack**: 100 packets/second per source
- **Attackers**: Heavily limited to 10 packets/second

### 2. Traffic Classification
- Distinguishes between legitimate and attack traffic
- Applies different policies based on classification
- Maintains service quality for legitimate users

### 3. Load Balancing
- Distributes traffic across multiple servers
- Provides redundancy and fault tolerance
- Prevents single points of failure

### 4. Queue Management
- RED (Random Early Detection) queuing
- Congestion control and buffer management
- Prevents buffer overflow attacks

## 🔬 Experimental Scenarios

### Scenario 1: Baseline Attack (No Defense)
```bash
./ns3 run "ddos-defense-simulation --enableDefense=false"
```
**Expected Results**: High packet loss, poor legitimate user experience

### Scenario 2: Defense Activation
```bash
./ns3 run "ddos-defense-simulation --enableDefense=true"
```
**Expected Results**: Preserved legitimate traffic, mitigated attack impact

### Scenario 3: Scalability Test
```bash
./ns3 run "ddos-defense-simulation --nAttackers=50 --nLegitimate=20 --nServers=10"
```
**Expected Results**: Test defense effectiveness under high load

### Scenario 4: Resource Limitation
```bash
./ns3 run "ddos-defense-simulation --nAttackers=15 --nServers=1"
```
**Expected Results**: Defense performance with limited resources

## 📈 Performance Analysis

### Key Metrics to Monitor
1. **Legitimate Traffic Success Rate**: Should remain >80% with defenses
2. **Attack Mitigation Rate**: Should block >90% of attack traffic
3. **Defense Activation Time**: How quickly threats are detected
4. **Resource Utilization**: Network capacity and server load

### Comparative Analysis
Run simulations with and without defenses to compare:
- Packet loss rates
- Throughput preservation
- Service availability
- Attack effectiveness

## 🐛 Troubleshooting

### Common Issues

#### Build Errors
```bash
# Clean and rebuild
./ns3 clean
./ns3 build
```

#### Memory Issues
```bash
# Reduce simulation scale
./ns3 run "ddos-defense-simulation --nAttackers=5 --simulationTime=20"
```

#### Animation Problems
```bash
# Check NetAnim installation
cd netanim
qmake --version
make clean && qmake NetAnim.pro && make
```

### Debug Mode
```bash
# Enable detailed logging
NS_LOG="DDoSDefenseSimulation=level_all" ./ns3 run ddos-defense-simulation
```

## 📚 Educational Applications

### Learning Objectives
- Understand DDoS attack mechanisms
- Learn network defense strategies
- Analyze traffic patterns and anomalies
- Evaluate security solution effectiveness

### Classroom Exercises
1. **Modify attack patterns** and observe defense responses
2. **Implement additional defense mechanisms** (honeypots, IP filtering)
3. **Analyze captured traffic** using Wireshark
4. **Compare different defense strategies** and their trade-offs

## 🤝 Contributing

### Extending the Simulation
- Add new attack vectors (SYN flood, HTTP flood)
- Implement advanced defense mechanisms
- Create more sophisticated traffic patterns
- Add machine learning-based detection

### Code Structure
```
ddos-defense-simulation.cc
├── RateLimitedUdpClient     # Custom application with rate limiting
├── DDoSDefenseManager       # Attack detection and response
├── Network Topology Setup   # Node and link configuration
├── Traffic Generation       # Attack and legitimate traffic
└── Analysis & Reporting     # Performance metrics
```

## 📄 License

This simulation is provided for educational and research purposes. Please ensure compliance with your institution's policies regarding network security research.

## 🆘 Support

### Resources
- [ns-3 Official Documentation](https://www.nsnam.org/documentation/)
- [ns-3 Tutorial](https://www.nsnam.org/docs/tutorial/html/)
- [Network Security Research Guidelines](https://www.nsnam.org/docs/manual/html/)

### Getting Help
1. Check the troubleshooting section
2. Review ns-3 documentation
3. Examine simulation logs for error messages
4. Verify all prerequisites are installed correctly

---

**⚠️ Important**: This simulation is designed for educational and research purposes only. Use responsibly and in accordance with ethical guidelines and legal requirements.
