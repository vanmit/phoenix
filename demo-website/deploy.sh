#!/bin/bash
# Phoenix Engine Demo Website Deployment Script
# Usage: ./deploy.sh

set -e

# Configuration
SERVER_HOST="47.245.126.212"
SERVER_PORT="3000"
SERVER_USER="admin"
SERVER_PATH="/var/www/showcase"
LOCAL_PATH="$(cd "$(dirname "$0")" && pwd)"

echo "🦅 Phoenix Engine Demo Website Deployment"
echo "=========================================="
echo ""
echo "Server: ${SERVER_USER}@${SERVER_HOST}:${SERVER_PORT}"
echo "Target: ${SERVER_PATH}"
echo ""

# Create tarball
echo "📦 Creating deployment package..."
cd "${LOCAL_PATH}"
tar -czf demo-website-deploy.tar.gz \
    index.html \
    demo-template.html \
    demos.json \
    css/ \
    js/ \
    demos/ \
    README.md

echo "✅ Package created: demo-website-deploy.tar.gz"
echo ""

# Upload to server
echo "📤 Uploading to server..."
if scp -P "${SERVER_PORT}" -o StrictHostKeyChecking=no demo-website-deploy.tar.gz ${SERVER_USER}@${SERVER_HOST}:/tmp/; then
    echo "✅ Upload successful"
    echo ""
    
    # Extract on server
    echo "📥 Extracting on server..."
    ssh -p "${SERVER_PORT}" -o StrictHostKeyChecking=no ${SERVER_USER}@${SERVER_HOST} << 'ENDSSH'
        mkdir -p /var/www/showcase
        tar -xzf /tmp/demo-website-deploy.tar.gz -C /var/www/showcase
        rm /tmp/demo-website-deploy.tar.gz
        echo "✅ Extraction complete"
ENDSSH
    
    echo ""
    echo "🎉 Deployment complete!"
    echo "📍 Website URL: http://${SERVER_HOST}:${SERVER_PORT}/showcase/"
else
    echo "❌ Upload failed"
    echo ""
    echo "Manual deployment instructions:"
    echo "1. Upload demo-website-deploy.tar.gz to server"
    echo "2. SSH to server: ssh -p ${SERVER_PORT} ${SERVER_USER}@${SERVER_HOST}"
    echo "3. Extract: tar -xzf demo-website-deploy.tar.gz -C /var/www/showcase"
    exit 1
fi

# Cleanup
rm -f demo-website-deploy.tar.gz

echo ""
echo "✨ All done! Visit http://${SERVER_HOST}:${SERVER_PORT}/showcase/"
