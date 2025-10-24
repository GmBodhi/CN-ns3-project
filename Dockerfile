FROM ubuntu:22.04

# Environment setup
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles

# Install comprehensive packages for ns-3 and NetAnim
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    g++ \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    cmake \
    git \
    wget \
    tar \
    pkg-config \
    # Qt5 for NetAnim (Ubuntu 22.04 compatible)
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    qt5-qmake \
    libqt5widgets5 \
    libqt5gui5 \
    libqt5core5a \
    libqt5network5 \
    libqt5xml5 \
    libqt5printsupport5 \
    # Additional Qt libraries
    qtchooser \
    # Virtual display for headless operation
    xvfb \
    x11-utils \
    # Analysis and debugging tools
    tcpdump \
    wireshark-common \
    gdb \
    vim \
    nano \
    htop \
    # X11 libs (for NetAnim even without display)
    libx11-dev \
    libxext-dev \
    # Clean up
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set Qt to run without display
ENV QT_QPA_PLATFORM=offscreen
ENV QT_X11_NO_MITSHM=1

# Set working directory
WORKDIR /home/ns3

# Download and extract ns-3
RUN wget https://www.nsnam.org/releases/ns-allinone-3.39.tar.bz2 && \
    tar -xjf ns-allinone-3.39.tar.bz2 && \
    rm ns-allinone-3.39.tar.bz2

# Build ns-3 with NetAnim support
WORKDIR /home/ns3/ns-allinone-3.39/ns-3.39
RUN ./ns3 configure \
    --build-profile=optimized \
    --enable-examples \
    --enable-tests \
    && ./ns3 build

# Build NetAnim
WORKDIR /home/ns3/ns-allinone-3.39/netanim-3.108
RUN ls -la && \
    if [ -f "NetAnim.pro" ]; then \
        echo "Building NetAnim..."; \
        qmake NetAnim.pro && \
        make -j$(nproc) && \
        echo "NetAnim built successfully"; \
    else \
        echo "NetAnim.pro not found, checking directory structure:"; \
        find /home/ns3/ns-allinone-3.39 -name "*netanim*" -type d; \
        find /home/ns3/ns-allinone-3.39 -name "*.pro" -type f; \
    fi

# Set final working directory
WORKDIR /home/ns3/ns-allinone-3.39/ns-3.39

# Create NetAnim helper script
RUN echo '#!/bin/bash\n\
# NetAnim helper script\n\
echo "=== NetAnim Status ==="\n\
NETANIM_DIR="/home/ns3/ns-allinone-3.39/netanim-3.108"\n\
if [ -f "$NETANIM_DIR/NetAnim" ]; then\n\
    echo "✅ NetAnim binary found at: $NETANIM_DIR/NetAnim"\n\
    echo "✅ NetAnim is ready to use"\n\
    echo\n\
    echo "To run NetAnim headless:"\n\
    echo "  xvfb-run -a $NETANIM_DIR/NetAnim <animation-file.xml>"\n\
    echo\n\
    echo "To analyze XML files:"\n\
    echo "  xmllint --format <animation-file.xml> | head -20"\n\
else\n\
    echo "❌ NetAnim binary not found"\n\
    echo "Available NetAnim files:"\n\
    find /home/ns3/ns-allinone-3.39 -name "*netanim*" -o -name "*NetAnim*"\n\
fi\n\
echo\n\
echo "To generate animation files, add this to your simulation:"\n\
echo "#include \"ns3/netanim-module.h\""\n\
echo "AnimationInterface anim (\"animation.xml\");"\n\
echo\n\
echo "Animation files will be saved to current directory"\n' > /netanim-status.sh && \
    chmod +x /netanim-status.sh

# Create entrypoint script
RUN echo '#!/bin/bash\n\
cd /home/ns3/ns-allinone-3.39/ns-3.39\n\
echo "=== ns-3 + NetAnim Environment ==="\n\
echo "ns-3 version: $(./ns3 --version 2>/dev/null || echo \"ns-3.39\")"\n\
echo "Working directory: $(pwd)"\n\
echo "NetAnim status:"\n\
/netanim-status.sh\n\
echo\n\
exec "$@"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]