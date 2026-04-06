#!/bin/bash
# Phoenix Engine WASM Fix Verification Script
# Runs comprehensive checks to verify the WASM loading issues are fixed

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "Phoenix Engine WASM Fix Verification"
echo "=========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass() { echo -e "${GREEN}✓${NC} $1"; }
fail() { echo -e "${RED}✗${NC} $1"; exit 1; }
warn() { echo -e "${YELLOW}!${NC} $1"; }
info() { echo "  $1"; }

# Check 1: File existence
echo "📁 Checking files..."
[ -f "phoenix-engine.wasm" ] && pass "phoenix-engine.wasm exists" || fail "phoenix-engine.wasm not found"
[ -f "phoenix-engine.js" ] && pass "phoenix-engine.js exists" || fail "phoenix-engine.js not found"
[ -f "demo-wasm-fixed.js" ] && pass "demo-wasm-fixed.js exists" || fail "demo-wasm-fixed.js not found"
[ -f "index.html" ] && pass "index.html exists" || fail "index.html not found"
[ -f "phoenix-server.py" ] && pass "phoenix-server.py exists" || fail "phoenix-server.py not found"
echo ""

# Check 2: WASM file validity
echo "🔍 Checking WASM file..."
WASM_MAGIC=$(xxd -l 4 -p phoenix-engine.wasm 2>/dev/null || echo "invalid")
if [ "$WASM_MAGIC" = "0061736d" ]; then
    pass "WASM magic number valid (0x00asm)"
else
    fail "Invalid WASM file (bad magic number: $WASM_MAGIC)"
fi

WASM_SIZE=$(stat -c%s phoenix-engine.wasm 2>/dev/null || stat -f%z phoenix-engine.wasm 2>/dev/null || echo "0")
info "WASM size: $WASM_SIZE bytes"

if [ "$WASM_SIZE" -gt 1000 ]; then
    pass "WASM file size reasonable"
else
    fail "WASM file too small (possibly corrupted)"
fi
echo ""

# Check 3: File permissions
echo "🔐 Checking permissions..."
[ -r "phoenix-engine.wasm" ] && pass "WASM file readable" || fail "WASM file not readable"
[ -x "phoenix-server.py" ] || chmod +x phoenix-server.py
[ -r "index.html" ] && pass "index.html readable" || fail "index.html not readable"
echo ""

# Check 4: HTML references correct JS
echo "📄 Checking HTML configuration..."
if grep -q "demo-wasm-fixed.js" index.html; then
    pass "index.html references fixed JS"
else
    warn "index.html does not reference demo-wasm-fixed.js"
fi

if grep -q 'src="demo-wasm.js"' index.html && ! grep -q '<!--.*demo-wasm.js' index.html; then
    warn "index.html still references old demo-wasm.js"
fi
echo ""

# Check 5: Server script validity
echo "🖥️  Checking server script..."
python3 -m py_compile phoenix-server.py && pass "phoenix-server.py syntax valid" || fail "phoenix-server.py has syntax errors"
echo ""

# Check 6: Port availability
echo "🌐 Checking port 8080..."
if command -v lsof &> /dev/null; then
    if lsof -i :8080 &> /dev/null; then
        warn "Port 8080 is already in use"
        info "Kill existing process: kill \$(lsof -t -i:8080)"
    else
        pass "Port 8080 is available"
    fi
elif command -v netstat &> /dev/null; then
    if netstat -tuln | grep -q ":8080 "; then
        warn "Port 8080 is already in use"
    else
        pass "Port 8080 is available"
    fi
else
    info "Cannot check port status (lsof/netstat not available)"
fi
echo ""

# Check 7: Browser availability (optional)
echo "🌍 Checking browser for automated testing..."
BROWSER_FOUND=false
if command -v google-chrome &> /dev/null; then
    info "Found: google-chrome"
    BROWSER_FOUND=true
elif command -v chromium &> /dev/null; then
    info "Found: chromium"
    BROWSER_FOUND=true
elif command -v firefox &> /dev/null; then
    info "Found: firefox"
    BROWSER_FOUND=true
else
    info "No browser found for automated testing"
fi
echo ""

# Summary
echo "=========================================="
echo "Verification Summary"
echo "=========================================="
echo ""
echo "All basic checks passed! 🎉"
echo ""
echo "Next steps:"
echo "  1. Start server: python3 phoenix-server.py 8080"
echo "  2. Open browser: http://localhost:8080"
echo "  3. Open DevTools (F12) and check Console"
echo "  4. Verify no errors in Network panel"
echo ""
echo "For detailed debugging, see: DEBUG-GUIDE.md"
echo ""

# Optional: Start server if requested
if [ "$1" = "--start" ]; then
    echo "🚀 Starting server..."
    python3 phoenix-server.py 8080
fi
