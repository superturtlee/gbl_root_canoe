# DO NOT USE abl_original.efi It is just intermediate result from patcher which contains the official abl efi progress which will boot efisp causing bootloop

# build with CLAND/LLD version 20
# GBL Root Canoe

## This Proj is not used to hack inside games or commit a crime, Use at ur OWN RISK ! 


## What this project is

`gbl_root_canoe` is built on EDK2 and includes an exploit lab path for GBL/UEFI secure boot chain manipulation.

## Why it matters

- Demonstrates how a vulnerable Qualcomm UEFI chain can be abused by replacing/patching image components.
- Supports automated extract/modify/rebuild for GBL blobs and ABL payloads.
- Provides tests for reproducible exploit validation and safe researcher workflows.

## Exploit Influenced Platforms

- Xiaomi 17 series && RedMi K90 ProMax
- Oneplus 15 && ace 6t
- Redmagic 11 series
- Nubia Z80 ultra

*** None Of the Scamsung Phones r influenced ***

## Core directories

- `edk2/`: EDK2 source + Qualcomm platform packages, build scripts.
- `tools/`: exploit utilities (`extractfv.py`, `patch_abl.c`), patch generator, binary helpers.
- `images/`: staging inputs/outputs for raw firmware images.
- `tests/`: validation use cases, end-to-end patch+build checks.
- `Conf/`: build target rules and patch configuration definitions.

## Quick setup

1. setup environment:

```bash
cd edk2
source edksetup.sh
```

2. install dependencies:

- clang/llvm, lld
- Python 3
- make/ninja

3. build baseline firmware:

```bash
cd edk2
build -p ArmPlatformPkg/ArmPlatformPkg.dsc -a AARCH64 -b RELEASE
```

4. run features/tests:

```bash
cd ../tests
./runall.sh
```

## Exploit workflow (Qualcomm UEFI)

1. Extract components from a GBL/UEFI image (`tools/extractfv.py`).
2. Locate vulnerable module (e.g., ABL path, boot policy handler).
3. Apply patch model from `tools/patch_abl.c` or `Conf/` rule script.
4. Repack the image in `images/` and re-sign if needed.
5. Deploy to lab board and observe root escalation path.

## Notes

- `gbl_root_canoe` is for security research: responsible disclosure and lab-only operations.
- Keep a clean copy of original image to avoid bricking hardware.

## License

See `LICENSE` and `edk2/License.txt` for terms. Qualcomm exploit code examples are for research and may require compliance with local law.
