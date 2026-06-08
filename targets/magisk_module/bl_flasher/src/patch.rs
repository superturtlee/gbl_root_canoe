use anyhow::Result;
use std::fs;

use crate::status::write_log;
use crate::util::{cmd_system, cmd_tool, moddir, runtime_dir};

pub fn patch_efisp(abl: &str, sfb: bool, debug: bool) -> Result<i32> {
    let rt = runtime_dir();
    let patch_log = rt.join("patch.log");

    for entry in fs::read_dir(&rt)? {
        let _ = fs::remove_file(entry?.path());
    }

    let status = cmd_tool("extractfv")
        .args(["-o", rt.to_str().unwrap_or("."), "-v", abl])
        .output()?;
    write_log(&format!(
        "extractfv: {}",
        String::from_utf8_lossy(&status.stdout).trim()
    ));

    let status = cmd_tool("patch_abl")
        .args([
            rt.join("LinuxLoader.efi").to_str().unwrap_or(""),
            rt.join("patched.efi").to_str().unwrap_or(""),
        ])
        .output()?;
    let _ = fs::write(&patch_log, &status.stdout);

    if !rt.join("patched.efi").exists() {
        write_log("Patch failed");
        return Ok(1);
    }

    if sfb {
        write_log("Injecting superfastboot...");
        let loader = moddir().join("loader.elf");
        if !loader.exists() {
            write_log("loader.elf missing");
            return Ok(1);
        }
        let status = cmd_tool("elf_inject")
            .args([
                loader.to_str().unwrap_or(""),
                rt.join("patched.efi").to_str().unwrap_or(""),
                rt.join("injected.dll").to_str().unwrap_or(""),
            ])
            .output()?;
        write_log(&format!(
            "elf_inject: {}",
            String::from_utf8_lossy(&status.stdout).trim()
        ));
        if !rt.join("injected.dll").exists() {
            write_log("Injection failed");
            return Ok(1);
        }
        let status = cmd_tool("GenFw")
            .args([
                "-e",
                "UEFI_APPLICATION",
                "-o",
                rt.join("patched.efi").to_str().unwrap_or(""),
                rt.join("injected.dll").to_str().unwrap_or(""),
            ])
            .output()?;
        write_log(&format!(
            "GenFw: {}",
            String::from_utf8_lossy(&status.stdout).trim()
        ));
        write_log("Injection complete");
    }

    if debug {
        write_log("Debug mode: processing only, no flash");
        return Ok(0);
    }

    let efisp = format!("{}/efisp", crate::util::by_name_dir());
    let status = cmd_system("blockdev")
        .args(["--setrw", &efisp])
        .status()?;
    if !status.success() {
        write_log("efisp setrw failed");
        return Ok(1);
    }

    let status = cmd_system("dd")
        .args([
            &format!("if={}", rt.join("patched.efi").to_str().unwrap_or("")),
            &format!("of={}", efisp),
            "bs=4M",
            "conv=fsync",
        ])
        .status()?;
    if !status.success() {
        write_log("efisp flash failed");
        return Ok(1);
    }

    let _ = cmd_system("sync").status();
    write_log("efisp flash ok");

    let log_content = fs::read_to_string(&patch_log).unwrap_or_default();
    if !log_content.contains("Warning: Failed to patch ABL GBL") {
        write_log("GBL vuln detected, skip BL flash");
        return Ok(2);
    }
    Ok(0)
}

pub fn detect_gbl_vulnerability(abl: &str) -> Result<i32> {
    let rt = runtime_dir();
    let patch_log = rt.join("patch.log");

    for entry in fs::read_dir(&rt)? {
        let _ = fs::remove_file(entry?.path());
    }

    let status = cmd_tool("extractfv")
        .args(["-o", rt.to_str().unwrap_or("."), "-v", abl])
        .output()?;
    write_log(&format!(
        "extractfv: {}",
        String::from_utf8_lossy(&status.stdout).trim()
    ));

    let status = cmd_tool("patch_abl")
        .args([
            rt.join("LinuxLoader.efi").to_str().unwrap_or(""),
            rt.join("patched.efi").to_str().unwrap_or(""),
        ])
        .output()?;
    let _ = fs::write(&patch_log, &status.stdout);

    if !rt.join("patched.efi").exists() {
        write_log("Patch failed");
        return Ok(1);
    }

    let log_content = fs::read_to_string(&patch_log).unwrap_or_default();
    if !log_content.contains("Warning: Failed to patch ABL GBL") {
        write_log("GBL vuln detected");
        return Ok(0);
    }
    write_log("No GBL vuln found");
    Ok(2)
}
