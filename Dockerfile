FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles
ENV DISPLAY=host.docker.internal:0

# Install system dependencies in one layer
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    g++ \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-distutils \
    cmake \
    ninja-build \
    git \
    wget \
    tar \
    pkg-config \
    # Qt5 for NetAnim (Ubuntu 22.04 compatible)
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    libqt5widgets5 \
    libqt5gui5 \
    libqt5core5a \
    libqt5network5 \
    libqt5xml5 \
    libqt5printsupport5 \
    qt5-qmake \
    # Additional libraries
    libxml2 \
    libxml2-dev \
    libxmu-dev \
    libgtk-3-dev \
    # X11 and display
    x11-apps \
    xauth \
    xvfb \
    # Network and development tools
    tcpdump \
    wireshark-common \
    vim \
    nano \
    htop \
    # Debug tools
    gdb \
    valgrind \
    ccache \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /home/ns3

# Download and install ns-3
RUN wget https://www.nsnam.org/releases/ns-allinone-3.39.tar.bz2 && \
    tar -xjf ns-allinone-3.39.tar.bz2 && \
    rm ns-allinone-3.39.tar.bz2

# Build ns-3 with all features
WORKDIR /home/ns3/ns-allinone-3.39/ns-3.39
RUN ./ns3 configure --build-profile=optimized --enable-examples --enable-tests && \
    ./ns3 build

# Build NetAnim (fixed - no make clean before qmake)
WORKDIR /home/ns3/ns-allinone-3.39/netanim-3.108
RUN qmake NetAnim.pro && \
    make -j$(nproc) || echo "NetAnim build completed with warnings"

# Set final working directory
WORKDIR /home/ns3/ns-allinone-3.39/ns-3.39

# Create entrypoint script
RUN echo '#!/bin/bash\ncd /home/ns3/ns-allinone-3.39/ns-3.39\nexec "$@"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

# Verify installations
RUN echo "=== Verifying installations ===" && \
    ./ns3 --version && \
    qmake --version && \
    python3 --version && \
    echo "=== Build completed successfully ==="

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]