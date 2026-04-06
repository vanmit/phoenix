#!/usr/bin/env python3
"""
Phoenix Engine WASM Development Server
Provides correct MIME types for WASM and proper CORS headers
"""

import http.server
import socketserver
import os
import sys

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8080

# MIME types including WASM
MIME_TYPES = {
    '.wasm': 'application/wasm',
    '.js': 'application/javascript',
    '.mjs': 'application/javascript',
    '.css': 'text/css',
    '.html': 'text/html',
    '.json': 'application/json',
    '.glb': 'model/gltf-binary',
    '.gltf': 'model/gltf+json',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.gif': 'image/gif',
    '.webp': 'image/webp',
    '.svg': 'image/svg+xml',
    '.txt': 'text/plain',
    '.map': 'application/json',
}

class WASMHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers for WASM
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        
        # Add correct MIME type for WASM
        ext = os.path.splitext(self.path)[1].lower()
        if ext in MIME_TYPES:
            self.send_header('Content-Type', MIME_TYPES[ext])
        
        super().end_headers()
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()
    
    def log_message(self, format, *args):
        # Enhanced logging with MIME type info
        ext = os.path.splitext(args[0].split()[0])[1].lower()
        mime = MIME_TYPES.get(ext, 'unknown')
        print(f"[{self.log_date_time_string()}] {args[0]} -> MIME: {mime}")

def run_server():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    # Allow external connections (0.0.0.0 = all interfaces)
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("0.0.0.0", PORT), WASMHandler) as httpd:
        print(f"🚀 Phoenix Engine WASM Server")
        print(f"   URL: http://localhost:{PORT}")
        print(f"   Directory: {os.getcwd()}")
        print(f"   WASM MIME: application/wasm")
        print(f"   CORS: Enabled")
        print(f"   Cache: Disabled (development)")
        print(f"\n📋 Press Ctrl+C to stop")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n👋 Server stopped")

if __name__ == "__main__":
    run_server()
