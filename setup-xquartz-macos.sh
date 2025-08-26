#!/bin/bash
set -e

echo "=== XQuartz Setup for NetAnim on macOS ==="

# Check if XQuartz is installed
if ! command -v xquartz >/dev/null 2>&1; then
    echo "❌ XQuartz not found. Installing via Homebrew..."
    if ! command -v brew >/dev/null 2>&1; then
        echo "❌ Homebrew not found. Please install Homebrew first:"
        echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi
    brew install --cask xquartz
    echo "✅ XQuartz installed. Please log out and back in, then run this script again."
    exit 0
fi

echo "✅ XQuartz found"

# Check if XQuartz is running
if ! pgrep -x "Xquartz" > /dev/null; then
    echo "🚀 Starting XQuartz..."
    open -a XQuartz
    echo "⏳ Waiting for XQuartz to start..."
    sleep 3
    
    # Wait for XQuartz to be ready
    for i in {1..10}; do
        if pgrep -x "Xquartz" > /dev/null; then
            break
        fi
        sleep 1
    done
    
    if ! pgrep -x "Xquartz" > /dev/null; then
        echo "❌ XQuartz failed to start"
        exit 1
    fi
fi

echo "✅ XQuartz is running"

# Configure XQuartz for network connections
echo "🔧 Configuring XQuartz..."

# Enable network connections (required for Docker)
defaults write org.xquartz.X11 nolisten_tcp -bool false

# Allow connections from localhost and Docker
xhost + localhost
xhost + 127.0.0.1
xhost + host.docker.internal

# Create X11 authentication file for Docker
XAUTH_FILE="$HOME/.Xauth"
touch "$XAUTH_FILE"
xauth nlist "$DISPLAY" | sed -e 's/^..../ffff/' | xauth -f "$XAUTH_FILE" nmerge -

echo "✅ XQuartz configured for Docker"
echo "📁 X11 authentication file created: $XAUTH_FILE"

# Display current settings
echo ""
echo "📋 Current Configuration:"
echo "   DISPLAY: $DISPLAY"
echo "   XAUTHORITY: $XAUTH_FILE"
echo "   XQuartz process: $(pgrep -x Xquartz | head -1)"

echo ""
echo "🎉 Setup complete! You can now run:"
echo "   docker-compose up --build"
echo "   docker-compose exec ns3 bash"
echo "   # Inside container:"
echo "   cd netanim && ./NetAnim ../ddos-simulation.xml"

echo ""
echo "⚠️  Note: If you restart your Mac, you'll need to run this script again."