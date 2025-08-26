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

# Set DISPLAY if not already set (XQuartz default)
if [ -z "$DISPLAY" ]; then
    export DISPLAY=:0
    echo "🖥️  Set DISPLAY to :0"
fi

# Wait a moment for XQuartz to be fully ready
sleep 2

# Allow connections from localhost and Docker
echo "🔐 Configuring X11 access permissions..."
xhost + localhost 2>/dev/null || echo "   ⚠️  localhost access already configured"
xhost + 127.0.0.1 2>/dev/null || echo "   ⚠️  127.0.0.1 access already configured" 
xhost + host.docker.internal:0 2>/dev/null || echo "   ⚠️  host.docker.internal access already configured"

# Create X11 authentication file for Docker
XAUTH_FILE="$HOME/.Xauth"
echo "🔑 Creating X11 authentication file..."
touch "$XAUTH_FILE"

# Generate auth entry for Docker
if xauth nlist "$DISPLAY" >/dev/null 2>&1; then
    xauth nlist "$DISPLAY" | sed -e 's/^..../ffff/' | xauth -f "$XAUTH_FILE" nmerge - 2>/dev/null || true
    echo "   ✅ Authentication data added"
else
    echo "   ⚠️  No existing auth data found, creating basic entry"
    # Create a basic auth entry if none exists
    xauth -f "$XAUTH_FILE" add "$DISPLAY" . "$(mcookie)" 2>/dev/null || true
fi

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
