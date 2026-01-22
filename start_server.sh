#!/bin/bash
# Start Redis Clone Server (Linux)

echo "🚀 Starting Redis Clone Server..."
make clean
make
echo ""
echo "✅ Build complete. Starting server..."
./server_async
