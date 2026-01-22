#!/bin/bash
# Benchmark Redis Clone vs Redis

echo "======================================"
echo "  Redis Clone Benchmark (Linux)"
echo "======================================"
echo ""

# Check if redis-benchmark is available
if ! command -v redis-benchmark &> /dev/null; then
    echo "❌ redis-benchmark not found. Please install redis-tools:"
    echo "   sudo apt install redis-tools"
    exit 1
fi

echo "📊 Testing C++ Redis Clone (port 7379)..."
echo ""

echo "--- SET Performance ---"
redis-benchmark -p 7379 -t set -n 100000 -q

echo ""
echo "--- GET Performance ---"
redis-benchmark -p 7379 -t get -n 100000 -q

echo ""
echo "✅ Benchmark complete!"
