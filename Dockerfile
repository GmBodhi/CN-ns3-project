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
    build-essential \
    g++ \
    python3 \
    python3-dev \
    cmake \
    git \
    wget \
    tar \
    pkg-config \
    # Qt5 for NetAnim
    qtbase5-dev \
    qttools5-dev \
    libqt5widgets5 \
    libqt5gui5 \
    libqt5core5a \
    qt5-qmake \
    # X11 support
    x11-apps \
    xauth \
    libxml2 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /home/ns3

# Download and install ns-3
RUN wget https://www.nsnam.org/releases/ns-allinone-3.39.tar.bz2 && \
    tar -xjf ns-allinone-3.39.tar.bz2 && \
    rm ns-allinone-3.39.tar.bz2

# Build ns-3
WORKDIR /home/ns3/ns-allinone-3.39/ns-3.39
RUN ./ns3 configure --build-profile=optimized && \
    ./ns3 build

# Create simple entrypoint
RUN echo '#!/bin/bash\ncd /home/ns3/ns-allinone-3.39/ns-3.39\nexec "$@"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]