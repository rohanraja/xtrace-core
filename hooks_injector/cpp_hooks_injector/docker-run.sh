#!/bin/bash

# Build and run the C++ Hooks Injector Docker container

set -e

echo "Building C++ Hooks Injector Docker image..."
docker build -t cpp-hooks-injector:latest .

echo "Running C++ Hooks Injector container..."
docker run -d \
  --name cpp-hooks-injector \
  -p 3001:3001 \
  --restart unless-stopped \
  cpp-hooks-injector:latest

echo "Container started successfully!"
echo "Health check: curl http://localhost:3001/health"
echo "API endpoint: POST http://localhost:3001/inject"
echo ""
echo "To view logs: docker logs cpp-hooks-injector"
echo "To stop: docker stop cpp-hooks-injector"
echo "To remove: docker rm cpp-hooks-injector"
