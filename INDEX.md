# UEFI Reverse Engineering MCP Servers - Documentation Index

**Generated**: April 8, 2026  
**Status**: ✅ Complete  
**Recommendation**: **pyghidra-mcp** (5-10 min setup, $0 cost, full UEFI support)

---

## 📖 Documentation Files

### 1. **SUMMARY.md** ⭐ START HERE
**Length**: ~337 lines | **Read Time**: 5 minutes

Executive summary with:
- Quick recommendation (pyghidra-mcp)
- One-command installation
- Answers to all your questions
- Quick start procedures
- Final recommendation with rationale

**Best for**: Getting started quickly, understanding the recommendation

---

### 2. **QUICK_INSTALL_COMMANDS.md** 🚀 COPY-PASTE READY
**Length**: ~299 lines | **Read Time**: 3 minutes

Ready-to-run commands for:
- One-liner setup (pyghidra-mcp)
- Step-by-step installation
- First UEFI analysis
- UEFI-specific commands
- MCP configuration templates
- Troubleshooting commands
- Verification checklists

**Best for**: Running installation commands, quick reference

---

### 3. **UEFI_MCP_SETUP_GUIDE.md** 📚 COMPREHENSIVE REFERENCE
**Length**: ~544 lines | **Read Time**: 15 minutes

Detailed guide covering:
- Executive summary table
- Option 1: pyghidra-mcp (recommended)
  - Installation steps
  - Quick start
  - MCP configuration
  - UEFI-specific setup
  - Advantages/limitations
  - Troubleshooting
- Option 2: bethington/ghidra-mcp (production)
  - Installation steps
  - Docker deployment
  - MCP configuration
  - UEFI-specific setup
  - Advantages/limitations
  - Troubleshooting
- Option 3: Binary Ninja Headless MCP (not recommended)
  - Why not recommended
  - Installation (if you have license)
  - Advantages/disadvantages
- UEFI-specific comparison
- Final recommendation
- Quick start script

**Best for**: Deep understanding, detailed reference, troubleshooting

---

## 🎯 QUICK NAVIGATION

### I want to...

**Get started immediately** → Read **SUMMARY.md** (5 min)

**Copy-paste commands** → Use **QUICK_INSTALL_COMMANDS.md**

**Understand all options** → Read **UEFI_MCP_SETUP_GUIDE.md**

**Troubleshoot issues** → See **UEFI_MCP_SETUP_GUIDE.md** troubleshooting sections

**Configure MCP** → See **QUICK_INSTALL_COMMANDS.md** MCP Configuration section

---

## 🏆 RECOMMENDATION AT A GLANCE

### **pyghidra-mcp** (RECOMMENDED)

```bash
# One-command setup (5-10 minutes)
sudo apt update && sudo apt install -y python3.11 python3.11-venv python3-pip openjdk-21-jdk && \
python3.11 -m venv ~/.venv/pyghidra-mcp && \
source ~/.venv/pyghidra-mcp/bin/activate && \
pip install --upgrade pip && \
pip install pyghidra-mcp && \
mkdir -p ~/ghidra_projects/uefi_analysis && \
echo "✅ Setup complete!"
```

**Why**:
- ✅ Fastest setup (5-10 min)
- ✅ Zero cost (Apache 2.0)
- ✅ Full UEFI support (PE32+, AArch64, custom types)
- ✅ Headless-first (CLI + HTTP)
- ✅ Active development (v0.1.14, April 2026)

---

## 📊 COMPARISON TABLE

| Feature | pyghidra-mcp | bethington/ghidra-mcp | binary-ninja-mcp |
|---------|--------------|----------------------|------------------|
| **Setup Time** | 5-10 min | 20-30 min | 30+ min |
| **Cost** | $0 | $0 | $599+/year |
| **Headless** | ✅ Full | ✅ Full | ✅ Full |
| **UEFI Support** | ✅ YES | ✅ YES | ✅ YES |
| **AArch64 PE32+** | ✅ YES | ✅ YES | ✅ YES |
| **Type Libraries** | ✅ YES | ✅ YES | ✅ YES |
| **MCP Tools** | 150+ | 193 | 181 |
| **Status** | Beta | Production | Production |
| **Recommendation** | 🏆 BEST | 🔄 Backup | ❌ Not recommended |

---

## ✅ ANSWERS TO YOUR QUESTIONS

### Q: Does it support AArch64 PE32+ EFI binaries?
**A**: YES ✅ (All three options)

### Q: Can it load UEFI type libraries (.gdt files)?
**A**: YES ✅ (All three options)

### Q: Can it run fully headless (no X11/display)?
**A**: YES ✅ (All three options)

### Q: Does it require a GUI?
**A**: NO ✅ (All three are fully headless)

### Q: What's the MCP server configuration (stdio vs HTTP)?
**A**: Both supported ✅
- pyghidra-mcp: HTTP (default) or stdio
- bethington/ghidra-mcp: HTTP (default) or stdio
- binary-ninja-mcp: stdio (default) or TCP

### Q: Any known issues with UEFI analysis?
**A**: None reported ✅

---

## 🚀 GETTING STARTED

### Step 1: Choose Your Option
- **Fastest**: pyghidra-mcp (recommended)
- **Most Stable**: bethington/ghidra-mcp
- **Most Expensive**: binary-ninja-mcp (not recommended)

### Step 2: Install
- See **QUICK_INSTALL_COMMANDS.md** for copy-paste commands

### Step 3: Analyze UEFI Binary
```bash
# pyghidra-mcp
pyghidra-mcp --transport streamable-http --wait-for-analysis /path/to/firmware.efi

# bethington/ghidra-mcp
curl -X POST -d "file=/path/to/firmware.efi" http://localhost:8089/load_program
```

### Step 4: Query Results
```bash
# pyghidra-mcp
pyghidra-mcp-cli list-functions --server http://localhost:5000

# bethington/ghidra-mcp
curl http://localhost:8089/list_functions
```

---

## 📚 EXTERNAL REFERENCES

- **pyghidra-mcp**: https://github.com/clearbluejar/pyghidra-mcp
- **bethington/ghidra-mcp**: https://github.com/bethington/ghidra-mcp
- **binary-ninja-headless-mcp**: https://github.com/mrphrazer/binary-ninja-headless-mcp
- **Ghidra**: https://ghidra-sre.org/
- **Binary Ninja**: https://binary.ninja/
- **MCP Specification**: https://modelcontextprotocol.io/

---

## 📋 DOCUMENT CHECKLIST

- [x] Executive summary with recommendation
- [x] Exact installation steps (pip/docker)
- [x] MCP server configuration (stdio vs HTTP)
- [x] Headless operation verification
- [x] Custom type definitions (.gdt files)
- [x] AArch64 PE32+ support confirmation
- [x] Known issues documentation
- [x] Zero-cost options identified
- [x] Fastest setup option highlighted
- [x] Copy-paste ready commands
- [x] Troubleshooting guides
- [x] Verification checklists

---

## 🎯 FINAL RECOMMENDATION

**For UEFI reverse engineering on Linux (headless, zero-cost, fastest setup):**

### 🏆 Use: **pyghidra-mcp**

**Setup Time**: ~10 minutes  
**Cost**: $0  
**Headless**: ✅ Full  
**UEFI Ready**: ✅ Yes

**Next Step**: Read **SUMMARY.md** or jump to **QUICK_INSTALL_COMMANDS.md**

---

**Last Updated**: April 8, 2026  
**Status**: ✅ Complete and Ready to Use
