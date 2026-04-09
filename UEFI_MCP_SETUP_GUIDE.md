# UEFI Reverse Engineering MCP Servers: Comprehensive Setup Guide

**Date**: April 8, 2026  
**Target**: Linux (headless, no X11)  
**Existing Tools**: radare2 already installed  
**Goal**: Zero-cost, fastest setup, full headless operation

---

## EXECUTIVE SUMMARY

| Criteria | **Ghidra MCP (bethington)** | **pyghidra-mcp** | **Binary Ninja Headless MCP** |
|----------|---------------------------|-----------------|------------------------------|
| **Cost** | ✅ FREE (Apache 2.0) | ✅ FREE (Apache 2.0) | ❌ PAID ($599+/year) |
| **Headless** | ✅ YES (Docker-ready) | ✅ YES (CLI-first) | ✅ YES (requires license) |
| **Setup Time** | 🟡 15-30 min (Maven build) | ✅ 5-10 min (pip install) | ❌ 30+ min (license setup) |
| **UEFI Support** | ✅ YES (PE32+, AArch64) | ✅ YES (via pyghidra) | ✅ YES (via Binary Ninja) |
| **Type Libraries** | ✅ YES (import_data_types) | ✅ YES (pyghidra API) | ✅ YES (type_library tools) |
| **MCP Tools** | 193 tools | 150+ tools | 181 tools |
| **Maintenance** | ✅ Active (v5.0.0, Mar 2026) | ✅ Active (v0.1.14, Apr 2026) | ✅ Active (v0.2.0, Mar 2026) |
| **Docker Ready** | ✅ YES | ✅ YES | ✅ YES |
| **Production Ready** | ✅ YES | 🟡 BETA | ✅ YES |

---

## RECOMMENDATION: **pyghidra-mcp** (FASTEST SETUP)

**Why**: Fastest setup (5-10 min), zero cost, fully headless, active development, perfect for UEFI analysis.

**Setup Time**: ~10 minutes  
**Cost**: $0  
**Headless**: ✅ Full CLI + HTTP server  
**UEFI Support**: ✅ Full (PE32+, AArch64, custom type libraries)

---

## OPTION 1: pyghidra-mcp (RECOMMENDED - FASTEST)

### Overview
- **Repository**: https://github.com/clearbluejar/pyghidra-mcp
- **Latest Release**: v0.1.14 (April 2026)
- **License**: Apache 2.0 (FREE)
- **Status**: Beta but actively maintained
- **Architecture**: Pure Python + pyghidra (no GUI required)

### Installation (Linux)

```bash
# 1. Install system dependencies
sudo apt update && sudo apt install -y python3.11 python3.11-venv python3-pip openjdk-21-jdk

# 2. Create virtual environment
python3.11 -m venv ~/.venv/pyghidra-mcp
source ~/.venv/pyghidra-mcp/bin/activate

# 3. Install pyghidra-mcp (fastest method)
pip install pyghidra-mcp

# 4. Verify installation
pyghidra-mcp --version
```

**Total Time**: ~5-10 minutes (pip handles all dependencies)

### Quick Start (Headless)

```bash
# Terminal 1: Start the MCP server (HTTP mode)
source ~/.venv/pyghidra-mcp/bin/activate
pyghidra-mcp --transport streamable-http --wait-for-analysis /path/to/uefi_binary.efi

# Terminal 2: Use the CLI client
source ~/.venv/pyghidra-mcp/bin/activate
pyghidra-mcp-cli list-functions --server http://localhost:5000
```

### MCP Configuration (for Claude Code / other MCP hosts)

**File**: `~/.config/claude/mcp.json` (or your MCP host config)

```json
{
  "mcpServers": {
    "pyghidra_mcp": {
      "command": "python3.11",
      "args": [
        "-m",
        "pyghidra_mcp.server",
        "--transport",
        "stdio"
      ],
      "env": {
        "GHIDRA_INSTALL_DIR": "/opt/ghidra_12.0.3_PUBLIC"
      }
    }
  }
}
```

### UEFI-Specific Setup

#### 1. Load UEFI Type Libraries (.gdt files)

```bash
# Create a Ghidra project with UEFI types
pyghidra-mcp --project-path ~/ghidra_projects/uefi_analysis \
  --import-types /path/to/uefi_types.gdt \
  /path/to/firmware.efi
```

#### 2. Analyze AArch64 PE32+ EFI Binary

```bash
# pyghidra-mcp auto-detects architecture from PE header
pyghidra-mcp --wait-for-analysis /path/to/arm64_uefi.efi

# Verify architecture detection
pyghidra-mcp-cli get-metadata --server http://localhost:5000
# Output includes: architecture, processor, entry_point
```

#### 3. Import Custom UEFI Structures

```python
# Via Python API (if using pyghidra-mcp programmatically)
from pyghidra_mcp import GhidraMCPServer

server = GhidraMCPServer()
server.import_data_types(
    category="UEFI",
    gdt_file="/path/to/uefi_structures.gdt"
)
```

### Key Features for UEFI Analysis

| Feature | Support | Notes |
|---------|---------|-------|
| **PE32+ Parsing** | ✅ YES | Auto-detects from binary |
| **AArch64 (ARM64)** | ✅ YES | Full support via pyghidra |
| **Custom Type Libraries** | ✅ YES | Import .gdt files directly |
| **Headless Operation** | ✅ YES | No GUI required |
| **Batch Analysis** | ✅ YES | Multiple binaries in one project |
| **Cross-Binary Xrefs** | ✅ YES | Track calls across modules |

### Advantages
- ✅ **Fastest setup** (pip install, no Maven build)
- ✅ **Pure Python** (easier to debug/extend)
- ✅ **CLI-first design** (perfect for automation)
- ✅ **Project-wide analysis** (multiple binaries in one session)
- ✅ **Zero cost** (Apache 2.0)
- ✅ **Active development** (v0.1.14, April 2026)

### Limitations
- 🟡 **Beta status** (not production-hardened like bethington/ghidra-mcp)
- 🟡 **Fewer tools** (~150 vs 193 in bethington)
- 🟡 **Smaller community** (newer project)

### Troubleshooting

```bash
# If pyghidra import fails
pip install --upgrade pyghidra

# If Ghidra not found
export GHIDRA_INSTALL_DIR=/path/to/ghidra_12.0.3_PUBLIC
pyghidra-mcp --version

# Check server health
curl http://localhost:5000/health
```

---

## OPTION 2: Ghidra MCP (bethington fork) - PRODUCTION GRADE

### Overview
- **Repository**: https://github.com/bethington/ghidra-mcp
- **Latest Release**: v5.0.0 (March 2026)
- **License**: Apache 2.0 (FREE)
- **Status**: Production-ready
- **Architecture**: Java plugin + Python bridge (hybrid)

### Installation (Linux)

```bash
# 1. Install system dependencies
sudo apt update && sudo apt install -y \
  openjdk-21-jdk \
  maven \
  python3 \
  python3-pip \
  curl \
  jq \
  unzip

# 2. Download Ghidra 12.0.3
cd ~/tools
wget https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.0.3_build/ghidra_12.0.3_PUBLIC_20240410.zip
unzip ghidra_12.0.3_PUBLIC_20240410.zip

# 3. Clone ghidra-mcp
git clone https://github.com/bethington/ghidra-mcp.git
cd ghidra-mcp

# 4. Run setup script (Linux)
chmod +x ghidra-mcp-setup.sh
./ghidra-mcp-setup.sh --deploy --ghidra-path ~/tools/ghidra_12.0.3_PUBLIC

# 5. Install Python bridge dependencies
pip install -r requirements.txt
```

**Total Time**: ~20-30 minutes (Maven build + Ghidra download)

### Quick Start (Headless)

```bash
# Terminal 1: Start Ghidra headless server
cd ~/tools/ghidra_12.0.3_PUBLIC
./support/analyzeHeadless /tmp/ghidra_projects MyProject -import /path/to/uefi.efi

# Terminal 2: Start MCP bridge
cd ~/ghidra-mcp
python bridge_mcp_ghidra.py --transport stdio
```

### Docker Deployment (Recommended for Headless)

```bash
# Build Docker image
cd ghidra-mcp/docker
docker build -t ghidra-mcp-headless:latest ..

# Run container
docker run -d \
  --name ghidra-mcp \
  -p 8089:8089 \
  -v /tmp/ghidra_projects:/projects \
  -e JAVA_OPTS="-Xmx4g -XX:+UseG1GC" \
  ghidra-mcp-headless:latest

# Verify
curl http://localhost:8089/check_connection
```

### MCP Configuration

**File**: `~/.config/claude/mcp.json`

```json
{
  "mcpServers": {
    "ghidra_mcp": {
      "command": "python3",
      "args": [
        "/path/to/ghidra-mcp/bridge_mcp_ghidra.py",
        "--transport",
        "stdio"
      ],
      "env": {
        "GHIDRA_SERVER_URL": "http://127.0.0.1:8089/"
      }
    }
  }
}
```

### UEFI-Specific Setup

#### 1. Load UEFI Type Libraries

```bash
# Via Ghidra headless analyzer
./support/analyzeHeadless /tmp/ghidra_projects MyProject \
  -import /path/to/uefi.efi \
  -scriptPath /path/to/uefi_types \
  -postScript import_uefi_types.py
```

#### 2. Analyze AArch64 PE32+ EFI

```bash
# Ghidra auto-detects from PE header
curl -X POST \
  -d "file=/data/arm64_uefi.efi" \
  http://localhost:8089/load_program

# Verify architecture
curl http://localhost:8089/get_metadata | jq '.processor'
# Output: "AARCH64"
```

#### 3. Import Custom Type Definitions

```bash
# Via MCP tool
curl -X POST \
  -H "Content-Type: application/json" \
  -d '{
    "category": "UEFI",
    "gdt_file": "/data/uefi_structures.gdt"
  }' \
  http://localhost:8089/import_data_types
```

### Key Features for UEFI Analysis

| Feature | Support | Notes |
|---------|---------|-------|
| **PE32+ Parsing** | ✅ YES | Full support |
| **AArch64 (ARM64)** | ✅ YES | Ghidra 12.0.3+ |
| **Custom Type Libraries** | ✅ YES | `import_data_types` tool |
| **Headless Operation** | ✅ YES | Full headless server |
| **Batch Analysis** | ✅ YES | 93% fewer API calls |
| **Cross-Binary Xrefs** | ✅ YES | Full call graph support |
| **Function Hashing** | ✅ YES | SHA-256 cross-version matching |

### Advantages
- ✅ **Production-ready** (v5.0.0, battle-tested)
- ✅ **Most tools** (193 MCP tools)
- ✅ **Mature ecosystem** (1,200+ GitHub stars)
- ✅ **Docker-ready** (docker-compose included)
- ✅ **Zero cost** (Apache 2.0)
- ✅ **Batch operations** (93% API call reduction)

### Limitations
- 🟡 **Longer setup** (Maven build required)
- 🟡 **Heavier** (Java + Ghidra + Python)
- 🟡 **More complex** (hybrid architecture)

### Troubleshooting

```bash
# If Maven build fails
./ghidra-mcp-setup.sh --setup-deps --ghidra-path ~/tools/ghidra_12.0.3_PUBLIC

# If server won't start
curl http://127.0.0.1:8089/check_connection

# Check Ghidra logs
tail -f ~/.config/ghidra/ghidra_12.0.3_PUBLIC/logs/ghidra.log
```

---

## OPTION 3: Binary Ninja Headless MCP (NOT RECOMMENDED FOR UEFI)

### Overview
- **Repository**: https://github.com/mrphrazer/binary-ninja-headless-mcp
- **Latest Release**: v0.2.0 (March 2026)
- **License**: GPL-2.0 (server), but requires Binary Ninja license
- **Status**: Production-ready
- **Cost**: ❌ **$599+/year** (Commercial license required)

### Why NOT Recommended for UEFI

| Issue | Impact |
|-------|--------|
| **License Cost** | $599/year minimum (vs $0 for Ghidra) |
| **No Free Trial** | Personal license doesn't include headless |
| **UEFI Support** | ✅ Works, but less mature than Ghidra |
| **Type Libraries** | ✅ Supported, but fewer UEFI-specific tools |

### Installation (if you have a license)

```bash
# 1. Install Binary Ninja with headless license
# (requires valid license key)

# 2. Install MCP server
pip install git+https://github.com/mrphrazer/binary-ninja-headless-mcp.git

# 3. Start server
python3 binary_ninja_headless_mcp.py --transport stdio
```

### MCP Configuration

```json
{
  "mcpServers": {
    "binary_ninja_mcp": {
      "command": "python3",
      "args": [
        "/path/to/binary_ninja_headless_mcp.py",
        "--transport",
        "stdio"
      ]
    }
  }
}
```

### Advantages
- ✅ **181 tools** (comprehensive)
- ✅ **Headless-first design** (agent-optimized)
- ✅ **Production-ready** (v0.2.0)
- ✅ **Active development** (Tim Blazytko)

### Disadvantages
- ❌ **$599+/year cost** (dealbreaker for most)
- ❌ **No free tier** (even for testing)
- ❌ **Overkill for UEFI** (Ghidra is better for firmware)

---

## UEFI-SPECIFIC COMPARISON

### PE32+ Support

| Server | Support | Notes |
|--------|---------|-------|
| **pyghidra-mcp** | ✅ Full | Auto-detects from PE header |
| **bethington/ghidra-mcp** | ✅ Full | Ghidra 12.0.3+ native support |
| **binary-ninja-mcp** | ✅ Full | Binary Ninja PE parser |

### AArch64 (ARM64) Support

| Server | Support | Notes |
|--------|---------|-------|
| **pyghidra-mcp** | ✅ Full | pyghidra handles all architectures |
| **bethington/ghidra-mcp** | ✅ Full | Ghidra 12.0.3+ native support |
| **binary-ninja-mcp** | ✅ Full | Binary Ninja ARM64 support |

### Custom Type Libraries (.gdt files)

| Server | Support | Method |
|--------|---------|--------|
| **pyghidra-mcp** | ✅ YES | `import_data_types()` API |
| **bethington/ghidra-mcp** | ✅ YES | `import_data_types` MCP tool |
| **binary-ninja-mcp** | ✅ YES | `type_library.load()` tool |

### Example: Loading UEFI Type Library

#### pyghidra-mcp
```bash
pyghidra-mcp --project-path ~/uefi_analysis \
  --import-types /path/to/uefi_types.gdt \
  /path/to/firmware.efi
```

#### bethington/ghidra-mcp
```bash
curl -X POST \
  -H "Content-Type: application/json" \
  -d '{"category":"UEFI","gdt_file":"/path/to/uefi_types.gdt"}' \
  http://localhost:8089/import_data_types
```

#### binary-ninja-mcp
```bash
# Via MCP tool
{
  "tool": "type_library.load",
  "params": {
    "path": "/path/to/uefi_types.bntl"
  }
}
```

---

## FINAL RECOMMENDATION

### For UEFI Reverse Engineering on Linux (Headless)

**🏆 WINNER: pyghidra-mcp**

**Why**:
1. ✅ **Fastest setup** (5-10 min vs 20-30 min)
2. ✅ **Zero cost** (Apache 2.0)
3. ✅ **Full UEFI support** (PE32+, AArch64, custom types)
4. ✅ **Headless-first** (CLI + HTTP, no GUI needed)
5. ✅ **Active development** (v0.1.14, April 2026)
6. ✅ **Perfect for automation** (CLI client included)

**Setup Time**: ~10 minutes  
**Cost**: $0  
**Headless**: ✅ Full  
**UEFI Ready**: ✅ Yes

### Backup Option: bethington/ghidra-mcp

If you need production-grade stability and don't mind the longer setup:
- **Setup Time**: ~30 minutes
- **Cost**: $0
- **Headless**: ✅ Full (Docker-ready)
- **UEFI Ready**: ✅ Yes
- **Advantage**: 193 tools vs 150+, battle-tested

---

## QUICK START SCRIPT (pyghidra-mcp)

```bash
#!/bin/bash
set -e

echo "=== pyghidra-mcp UEFI Setup ==="

# 1. Install dependencies
echo "[1/5] Installing system dependencies..."
sudo apt update && sudo apt install -y python3.11 python3.11-venv python3-pip openjdk-21-jdk

# 2. Create venv
echo "[2/5] Creating virtual environment..."
python3.11 -m venv ~/.venv/pyghidra-mcp
source ~/.venv/pyghidra-mcp/bin/activate

# 3. Install pyghidra-mcp
echo "[3/5] Installing pyghidra-mcp..."
pip install --upgrade pip
pip install pyghidra-mcp

# 4. Verify installation
echo "[4/5] Verifying installation..."
pyghidra-mcp --version

# 5. Create UEFI project directory
echo "[5/5] Creating UEFI project directory..."
mkdir -p ~/ghidra_projects/uefi_analysis

echo ""
echo "✅ Setup complete!"
echo ""
echo "Next steps:"
echo "1. Activate venv: source ~/.venv/pyghidra-mcp/bin/activate"
echo "2. Start server: pyghidra-mcp --transport streamable-http --wait-for-analysis /path/to/firmware.efi"
echo "3. In another terminal: pyghidra-mcp-cli list-functions --server http://localhost:5000"
echo ""
```

---

## REFERENCES

- **pyghidra-mcp**: https://github.com/clearbluejar/pyghidra-mcp
- **bethington/ghidra-mcp**: https://github.com/bethington/ghidra-mcp
- **binary-ninja-headless-mcp**: https://github.com/mrphrazer/binary-ninja-headless-mcp
- **Ghidra**: https://ghidra-sre.org/
- **Binary Ninja**: https://binary.ninja/
- **MCP Specification**: https://modelcontextprotocol.io/

