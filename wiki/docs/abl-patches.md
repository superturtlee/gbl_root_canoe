# ABL Patch Details

This page documents the ABL changes currently performed by `patch_abl`. It is based on the implementation in `submodules/patcher/src/patchs/`.

## Patch Flow

The PC toolkit first extracts the largest EFI/PE payload from the stock ABL image as `ABL_original.efi`, then runs:

```bash
patch_abl ABL_original.efi ABL.efi
```

`patch_abl` edits the buffer in place and writes the patched output. It does not resize the EFI, repack sections, or sign the output.

## Applied Patches

| Area | What is changed | Purpose | Failure behavior |
|------|-----------------|---------|------------------|
| GBL partition string | UTF-16LE `efisp` is replaced with UTF-16LE `nulls` | Prevent the vulnerable GBL path from using the original `efisp` name | Non-fatal warning: `Failed to patch ABL GBL` |
| `androidboot.vbmeta.device_state` | The ADRP+ADD pair that points to `unlocked` is rewritten to point to `locked` | Report `androidboot.vbmeta.device_state=locked` instead of `unlocked` | Non-fatal if not found; fatal if multiple matching triples are patched |
| Boot-state decision pattern | A matched ARM64 pattern has the `MOV W8, #1` immediate changed to `MOV W8, #0` | Force the matched boot-state branch path toward the locked value | Fatal if the pattern is not found |
| Lock-state source load | The tracked `LDRB` source instruction is replaced with `MOV Wn, #1` | Force the in-register lock-state source value used by later code | Non-fatal warning if the data-flow chain is not found |
| Lock-state sink store | The first tracked `STRB` sink after the boot-state anchor is changed to store `WZR` | Prevent the derived unlock state from being stored back to the tracked byte variable | Non-fatal warning if the sink is not found |
| Orange State warning | A nearby `CBZ Wn` guarding the warning text is rewritten to `CBZ WZR` after verifying the same lock-state source variable | Suppress the unlocked boot warning text | Non-fatal warning if not found |

## GBL String Patch

The patcher searches for this UTF-16LE byte sequence:

```text
```

It replaces the first match with:

```text
n\0 u\0 l\0 l\0 s\0
```

The replacement is the same length as the original string, so no layout changes are required.

## Device State Patch

The patcher scans ARM64 code for two consecutive ADRP+ADD address calculations that point to the strings `unlocked` and `locked`. It also checks that a nearby ADRP+ADD points to `androidboot.vbmeta.device_state`.

When this triple is found, the first ADRP+ADD pair is rewritten to use the target address of `locked`. This changes the selected string without editing the strings themselves.

If no triple is found, patching continues. If more than one triple is patched, `patch_abl` aborts because that could indicate an ambiguous match.

## Boot State And Lock Variable Patch

The patcher searches for a fixed 32-byte ARM64 instruction pattern around the boot-state logic. Some bytes are wildcards because register allocation can vary between ABL builds.

After a match:

1. It records the lock-state register from the matched conditional branch instruction.
2. It changes the matched `MOV W8, #1` encoding to `MOV W8, #0`.
3. It tracks backward from the matched anchor to find the `LDRB` that loads the lock-state byte from a global structure.
4. It replaces that source load with `MOV Wn, #1`.
5. It tracks forward from the source to the next relevant `STRB` sink and changes that sink to use `WZR`, causing a zero byte to be stored.

This combination forces the code path and stored state used by the patched ABL toward the locked-state behavior expected by the project.

## Warning Suppression Patch

For OPlus-style ABL builds, the patcher searches for references to both strings:

```text
Orange State
Your device has been unlocked and can't be trusted
```

It then scans backward in the same function for a `CBZ Wn` branch. The branch is patched only if reverse tracking confirms that `Wn` comes from the same lock-state global variable found earlier.

The instruction is rewritten as `CBZ WZR`. Since `WZR` is always zero, this forces the branch condition in the direction used to skip or disable the warning path.

## Testing-Only Patch

There is a fastboot-related patch implementation in `patchs/oplus/forceenablefastboot.c`. It looks for `fastboot_unlock_verify error and reboot.` and can rewrite a `CBZ`/`CBZ X` to an unconditional branch.

This patch is guarded by `ENABLE_TESTING_PATCHS` and is not enabled by the current Makefile builds.

## What Is Not Changed

`patch_abl` does not perform these operations:

1. It does not change file size.
2. It does not rebuild PE/EFI sections.
3. It does not resign the output.
4. It does not patch every possible vendor warning implementation.
5. It does not guarantee compatibility when pattern matching is ambiguous or missing.

Always check `patch_log.txt` after running the toolkit. Warnings such as `Failed to patch ABL GBL`, `ADRL triple not found`, or `patch_warning failed` indicate that part of the expected patch set was not applied.
