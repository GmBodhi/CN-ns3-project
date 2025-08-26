# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a DDoS (Distributed Denial of Service) attack and defense simulation project built using the ns-3 network simulator. The project demonstrates network security concepts through simulated attacks and defensive mechanisms for educational and research purposes.

## Build and Run Commands

### Docker Development Environment

**For macOS with NetAnim GUI support:**
```bash
# First-time setup (run on macOS host)
./setup-xquartz-macos.sh

# Build and start the ns-3 environment
docker-compose up --build

# Run interactive shell in container
docker-compose exec ns3 bash
```

### Native ns-3 Commands (inside container or native install)

**Build the simulation:**
```bash
./ns3 build
```

**Run the simulation with default parameters:**
```bash
./ns3 run "scratch/simulation/main --enableAttack=true"
```

**Run simulation without attack (baseline):**
```bash
./ns3 run "scratch/simulation/main --enableAttack=false"
```

**Run with custom parameters:**
```bash
./ns3 run "scratch/simulation/main --nAttackers=8 --nLegitimate=4 --simulationTime=30 --enableAttack=true"
```

**View NetAnim visualization:**
```bash
# Use the helper script (automatically finds animation files)
./launch-netanim.sh

# Or manually (after running simulation)
cd netanim
./NetAnim ../ddos-simulation.xml
```

**Clean build:**
```bash
./ns3 clean
./ns3 build
```

## Architecture

### Core Components

- **simulation/main.cc**: Main simulation entry point containing the DDoS simulation logic
- **Dockerfile**: Container setup with ns-3.39, Qt5 for NetAnim, and development tools
- **docker-compose.yml**: Development environment orchestration

### Network Topology

The simulation creates a simple but effective network topology:
- **Attackers**: Configurable number of nodes generating high-rate traffic
- **Legitimate Clients**: Normal users with typical traffic patterns
- **Router**: Central routing node handling all traffic
- **Server**: Single target server with bottleneck link (2Mb/s default)

### Key Simulation Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `nAttackers` | 5 | Number of attacking nodes |
| `nLegitimate` | 3 | Number of legitimate clients |
| `simulationTime` | 20.0s | Total simulation duration |
| `enableAttack` | true | Enable/disable attack traffic |

### NetAnim Visualization

The simulation generates `ddos-simulation.xml` for network animation with:
- **Red nodes**: Attackers (high-rate traffic)
- **Green nodes**: Legitimate clients (normal traffic)  
- **Blue node**: Router (central hub)
- **Orange node**: Server (target)

### Traffic Patterns

- **Legitimate Traffic**: 10 packets/second per client, 512-byte packets
- **Attack Traffic**: 1000 packets/second per attacker, 1024-byte packets  
- **Attack Duration**: 5s - 15s (10 second attack window)
- **Bottleneck**: Server link limited to 2Mbps

## File Structure

```
CN-ns3-project/
├── simulation/
│   └── main.cc              # Main simulation code
├── Dockerfile               # ns-3 environment setup
├── docker-compose.yml       # Development environment
└── README.md               # Comprehensive documentation
```

## Generated Output Files

When running simulations, these files are created:
- `ddos-simulation.xml`: NetAnim animation file for network visualization
- Console output with flow statistics and success rates

## Development Workflow

1. **Make code changes** to `simulation/main.cc`
2. **Build simulation**: `./ns3 build`
3. **Run simulation**: `./ns3 run "scratch/simulation/main [options]"`
4. **View animation**: Open `ddos-simulation.xml` with NetAnim
5. **Analyze results** using console output and animation

## macOS NetAnim Setup

### Prerequisites
1. **Install Homebrew** (if not already installed):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Install XQuartz**:
   ```bash
   brew install --cask xquartz
   ```

3. **Run the setup script** (first time only):
   ```bash
   chmod +x setup-xquartz-macos.sh
   ./setup-xquartz-macos.sh
   ```

### Troubleshooting
- If NetAnim doesn't open: Restart XQuartz and run setup script again
- If "cannot connect to display" error: Check that XQuartz allows network connections
- For performance issues: Close other GUI applications while running NetAnim

## Important Notes

- This simulation is designed for **educational and defensive security research only**
- Qt + XQuartz integration provides native GUI support on macOS
- All traffic uses standard ns-3 UDP echo applications  
- Visual network topology with color-coded node types