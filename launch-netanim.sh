#!/bin/bash
# NetAnim launcher script for ns-3 container
set -e

echo "=== NetAnim Launcher ==="

# Check if we're in the right directory
if [ ! -f "ns3" ]; then
    echo "❌ Error: ns3 script not found. Are you in the ns-3.39 directory?"
    exit 1
fi

# Check if NetAnim is built
if [ ! -f "netanim/NetAnim" ]; then
    echo "🔧 Building NetAnim..."
    cd netanim
    if [ ! -f "NetAnim.pro" ]; then
        echo "❌ Error: NetAnim.pro not found"
        exit 1
    fi
    
    # Clean and build NetAnim
    make clean 2>/dev/null || true
    qmake NetAnim.pro
    make
    
    if [ ! -f "NetAnim" ]; then
        echo "❌ Error: NetAnim build failed"
        exit 1
    fi
    
    echo "✅ NetAnim built successfully"
    cd ..
fi

# Look for animation XML files
echo "🔍 Looking for animation files..."
XML_FILES=(*.xml)
if [ ${#XML_FILES[@]} -eq 0 ] || [ ! -f "${XML_FILES[0]}" ]; then
    echo "⚠️  No .xml animation files found in current directory"
    echo "   Run a simulation first to generate ddos-simulation.xml"
    echo "   Example: ./ns3 run 'scratch/simulation/main --enableAttack=true'"
    echo ""
    echo "   Available files:"
    ls -la *.xml 2>/dev/null || echo "   (none)"
    exit 1
fi

# Show available animation files
echo "📁 Found animation files:"
for i in "${!XML_FILES[@]}"; do
    echo "   $((i+1)). ${XML_FILES[i]}"
done

# Use the first XML file or let user choose
ANIM_FILE="${XML_FILES[0]}"
if [ ${#XML_FILES[@]} -gt 1 ]; then
    echo ""
    echo "🎯 Using: $ANIM_FILE"
    echo "   (To use a different file, specify it as argument: $0 filename.xml)"
fi

# Allow command line argument to specify file
if [ $# -gt 0 ]; then
    if [ -f "$1" ]; then
        ANIM_FILE="$1"
        echo "🎯 Using specified file: $ANIM_FILE"
    else
        echo "❌ Error: File '$1' not found"
        exit 1
    fi
fi

# Check X11 connection
echo "🔍 Checking X11 connection..."
if [ -z "$DISPLAY" ]; then
    echo "❌ Error: DISPLAY variable not set"
    exit 1
fi

# Test X11 connection
if command -v xset >/dev/null 2>&1; then
    if ! xset q >/dev/null 2>&1; then
        echo "❌ Error: Cannot connect to X11 display $DISPLAY"
        echo "   Make sure XQuartz is running on the host and properly configured"
        exit 1
    fi
    echo "✅ X11 connection OK"
else
    echo "⚠️  xset not available, skipping X11 test"
fi

# Launch NetAnim
echo "🚀 Launching NetAnim with $ANIM_FILE..."
echo "   Display: $DISPLAY"
echo "   X11 Auth: ${XAUTHORITY:-not set}"

cd netanim
exec ./NetAnim "../$ANIM_FILE"