# Quick Install Commands - UEFI MCP Servers

## RECOMMENDED: pyghidra-mcp (5-10 minutes)

### One-Liner Setup
```bash
sudo apt update && sudo apt install -y python3.11 python3.11-venv python3-pip openjdk-21-jdk && \
python3.11 -m venv ~/.venv/pyghidra-mcp && \
source ~/.venv/pyghidra-mcp/bin/activate && \
pip install --upgrade pip && \
pip install pyghidra-mcp && \
mkdir -p ~/ghidra_projects/uefi_analysis && \
echo "✅ Setup complete! Run: source ~/.venv/pyghidra-mcp/bin/activate"
```

### Step-by-Step
```bash
# 1. Install system dependencies
sudo apt update && sudo apt install -y python3.11 python3.11-venv python3-pip openjdk-21-jdk

# 2. Create virtual environment
python3.11 -m venv ~/.venv/pyghidra-mcp
source ~/.venv/pyghidra-mcp/bin/activate

# 3. Install pyghidra-mcp
pip install --upgrade pip
pip install pyghidra-mcp

# 4. Verify
pyghidra-mcp --version

# 5. Create project directory
mkdir -p ~/ghidra_projects/uefi_analysis
```

### First Analysis (UEFI Binary)
```bash
# Terminal 1: Start server
source ~/.venv/pyghidra-mcp/bin/activate
pyghidra-mcp --transport streamable-http --wait-for-analysis /path/to/firmware.efi

# Terminal 2: Query results
source ~/.venv/pyghidra-mcp/bin/activate
pyghidra-mcp-cli list-functions --server http://localhost:5000
pyghidra-mcp-cli get-metadata --server http://localhost:5000
```

---

## ALTERNATIVE: bethington/ghidra-mcp (20-30 minutes)

### Prerequisites
```bash
sudo apt update && sudo apt install -y \
  openjdk-21-jdk \
  maven \
  python3 \
  python3-pip \
  curl \
  jq \
  unzip \
  git
```

### Download & Setup
```bash
# 1. Download Ghidra 12.0.3
cd ~/tools
wget https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.0.3_build/ghidra_12.0.3_PUBLIC_20240410.zip
unzip ghidra_12.0.3_PUBLIC_20240410.zip

# 2. Clone ghidra-mcp
git clone https://github.com/bethington/ghidra-mcp.git
cd ghidra-mcp

# 3. Run setup script
chmod +x ghidra-mcp-setup.sh
./ghidra-mcp-setup.sh --deploy --ghidra-path ~/tools/ghidra_12.0.3_PUBLIC

# 4. Install Python dependencies
pip install -r requirements.txt
```

### Docker Setup (Recommended for Headless)
```bash
# Build image
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

### First Analysis (UEFI Binary)
```bash
# Load binary
curl -X POST -d "file=/path/to/firmware.efi" http://localhost:8089/load_program

# Run analysis
curl -X POST http://localhost:8089/run_analysis

# List functions
curl "http://localhost:8089/list_functions?limit=20"

# Get metadata
curl http://localhost:8089/get_metadata | jq '.'
```

---

## UEFI-SPECIFIC COMMANDS

### Load UEFI Type Library (pyghidra-mcp)
```bash
source ~/.venv/pyghidra-mcp/bin/activate
pyghidra-mcp --project-path ~/ghidra_projects/uefi_analysis \
  --import-types /path/to/uefi_types.gdt \
  /path/to/firmware.efi
```

### Load UEFI Type Library (bethington/ghidra-mcp)
```bash
curl -X POST \
  -H "Content-Type: application/json" \
  -d '{
    "category": "UEFI",
    "gdt_file": "/path/to/uefi_types.gdt"
  }' \
  http://localhost:8089/import_data_types
```

### Analyze AArch64 PE32+ EFI (pyghidra-mcp)
```bash
source ~/.venv/pyghidra-mcp/bin/activate
pyghidra-mcp --wait-for-analysis /path/to/arm64_uefi.efi

# Verify architecture
pyghidra-mcp-cli get-metadata --server http://localhost:5000 | grep -i arch
```

### Analyze AArch64 PE32+ EFI (bethington/ghidra-mcp)
```bash
curl -X POST -d "file=/path/to/arm64_uefi.efi" http://localhost:8089/load_program
curl http://localhost:8089/get_metadata | jq '.processor'
# Expected output: "AARCH64"
```

---

## MCP CONFIGURATION

### For Claude Code (pyghidra-mcp)
```bash
# Create config file
mkdir -p ~/.config/claude
cat > ~/.config/claude/mcp.json << 'MCONFIG'
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
MCONFIG
```

### For Claude Code (bethington/ghidra-mcp)
```bash
# Create config file
mkdir -p ~/.config/claude
cat > ~/.config/claude/mcp.json << 'MCONFIG'
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
MCONFIG
```

---

## TROUBLESHOOTING

### pyghidra-mcp Issues

```bash
# Check if installed
pip list | grep pyghidra

# Upgrade pyghidra
pip install --upgrade pyghidra

# Check Ghidra path
export GHIDRA_INSTALL_DIR=/path/to/ghidra_12.0.3_PUBLIC
pyghidra-mcp --version

# Check server health
curl http://localhost:5000/health

# View logs
tail -f ~/.pyghidra-mcp/logs/server.log
```

### bethington/ghidra-mcp Issues

```bash
# Check if Maven build succeeded
ls -la ~/.m2/repository/com/xebyte/

# Check if server is running
curl http://127.0.0.1:8089/check_connection

# View Ghidra logs
tail -f ~/.config/ghidra/ghidra_12.0.3_PUBLIC/logs/ghidra.log

# Check if port is in use
lsof -i :8089
```

### Binary Ninja Issues

```bash
# Check if Binary Ninja is installed
python3 -c "import binaryninja; print(binaryninja.__version__)"

# Check if license is valid
python3 -c "from binaryninja import core; print(core.is_headless_license())"

# Test MCP server
python3 binary_ninja_headless_mcp.py --fake-backend
```

---

## VERIFICATION CHECKLIST

### pyghidra-mcp
- [ ] `pyghidra-mcp --version` returns version
- [ ] `pyghidra-mcp --transport streamable-http --wait-for-analysis /tmp/test.bin` starts without errors
- [ ] `curl http://localhost:5000/health` returns 200
- [ ] `pyghidra-mcp-cli list-functions --server http://localhost:5000` returns functions

### bethington/ghidra-mcp
- [ ] `curl http://localhost:8089/check_connection` returns "Connected"
- [ ] `curl http://localhost:8089/get_version` returns version
- [ ] `curl http://localhost:8089/list_functions` returns function list
- [ ] Docker container is running: `docker ps | grep ghidra-mcp`

### Binary Ninja Headless MCP
- [ ] `python3 binary_ninja_headless_mcp.py --version` returns version
- [ ] `python3 -c "import binaryninja; print('OK')"` returns OK
- [ ] License check: `python3 -c "from binaryninja import core; print(core.is_headless_license())"`

---

## NEXT STEPS

1. **Choose your MCP server**: pyghidra-mcp (fastest) or bethington/ghidra-mcp (most stable)
2. **Run installation**: Use commands above
3. **Load UEFI binary**: Use UEFI-specific commands
4. **Configure MCP host**: Add to Claude Code or other MCP client
5. **Start analyzing**: Use CLI or MCP tools

---

## REFERENCES

- **pyghidra-mcp**: https://github.com/clearbluejar/pyghidra-mcp
- **bethington/ghidra-mcp**: https://github.com/bethington/ghidra-mcp
- **Ghidra**: https://ghidra-sre.org/
- **MCP Spec**: https://modelcontextprotocol.io/

