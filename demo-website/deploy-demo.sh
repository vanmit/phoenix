#!/bin/bash
# Phoenix Engine Demo - Deployment Script
# Deploys to: 47.245.126.212:3000/demo/

set -e

SERVER="47.245.126.212"
DEPLOY_PATH="/var/www/html/demo"
SOURCE_PATH="./demo-showcase"

echo "🦅 Phoenix Engine - Demo Deployment"
echo "==================================="
echo ""

# Check if source directory exists
if [ ! -d "$SOURCE_PATH" ]; then
    echo "❌ Error: Source directory '$SOURCE_PATH' not found"
    exit 1
fi

# Build WASM if build.sh exists
if [ -f "$SOURCE_PATH/build.sh" ]; then
    echo "🔨 Building WASM module..."
    cd "$SOURCE_PATH"
    bash build.sh
    cd ..
fi

# Create dist directory for deployment
DEPLOY_DIR="./demo-showcase/dist"
if [ ! -d "$DEPLOY_DIR" ]; then
    echo "❌ Error: Build output directory not found"
    echo "   Please run: cd demo-showcase && bash build.sh"
    exit 1
fi

echo ""
echo "📦 Deployment Details:"
echo "   Server: $SERVER"
echo "   Path: $DEPLOY_PATH"
echo "   Source: $DEPLOY_DIR"
echo ""

# Check if rsync is available
if ! command -v rsync &> /dev/null; then
    echo "❌ rsync not found. Please install rsync."
    exit 1
fi

# Deploy using rsync
echo "🚀 Deploying to server..."
rsync -avz --delete \
    -e "ssh -o StrictHostKeyChecking=no" \
    "$DEPLOY_DIR/" \
    "root@$SERVER:$DEPLOY_PATH/"

echo ""
echo "✅ Deployment complete!"
echo ""
echo "📱 Access the demo at:"
echo "   http://$SERVER:3000/demo/"
echo ""
echo "🌐 Demo Center:"
echo "   http://$SERVER:3000/demo-center.html"
echo ""

# Copy demo-center.html as well
echo "📋 Deploying demo center..."
rsync -avz \
    -e "ssh -o StrictHostKeyChecking=no" \
    "./demo-center.html" \
    "root@$SERVER:/var/www/html/"

echo ""
echo "🎉 All done!"
