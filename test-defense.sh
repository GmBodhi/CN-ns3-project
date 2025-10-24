#!/bin/bash
echo "=== Building simulation ==="
docker-compose exec -T ns3 bash -c "cd /home/ns3/ns-allinone-3.39/ns-3.39 && ./ns3 clean && ./ns3 build"

echo ""
echo "=== Running simulation with defense ==="
docker-compose exec -T ns3 bash -c "cd /home/ns3/ns-allinone-3.39/ns-3.39 && ./ns3 run 'scratch/simulation/main --enableAttack=true --enableDefense=true --simulationTime=20'"
