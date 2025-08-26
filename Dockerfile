FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles
ENV DISPLAY=host.docker.internal:0
ENV QT_X11_NO_MITSHM=1
ENV QT_GRAPHICSSYSTEM=native
ENV XAUTHORITY=/root/.Xauth

# Install system dependencies
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


# Set final working directory
WORKDIR /home/ns3/ns-allinone-3.39/ns-3.39

# Create entrypoint script with X11 setup
RUN echo '#!/bin/bash\n\
cd /home/ns3/ns-allinone-3.39/ns-3.39\n\
# Set up X11 authentication if available\n\
if [ -f "$XAUTHORITY" ]; then\n\
    echo "Using X11 authentication from $XAUTHORITY"\n\
else\n\
    echo "Warning: X11 authentication file not found. GUI applications may not work."\n\
fi\n\
# Test X11 connection\n\
if command -v xset >/dev/null 2>&1; then\n\
    if xset q >/dev/null 2>&1; then\n\
        echo "X11 connection successful"\n\
    else\n\
        echo "Warning: Cannot connect to X11 display $DISPLAY"\n\
    fi\n\
fi\n\
exec "$@"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

# Verify installations (using correct commands)
RUN echo "=== Verifying installations ===" && \
    echo "ns-3 configuration:" && \
    ./ns3 show config | head -10 && \
    echo "Qt version:" && \
    qmake --version && \
    echo "Python version:" && \
    python3 --version && \
    echo "=== Build completed successfully ==="

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]