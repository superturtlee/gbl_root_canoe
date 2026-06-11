use anyhow::{bail, Result};
use serde_json::json;
use std::sync::OnceLock;
use std::fs;
use std::process::{Command, Stdio};

use crate::patch::{detect_gbl_vulnerability, patch_efisp};
use crate::status::{write_log, write_state};
use crate::util::{
    acquire_lock, current_pid, detect_current_slot, ensure_runtime, other_slot,
    partition_path, release_lock, runtime_dir, set_path_env,
};

static CTRLC_SET: OnceLock<()> = OnceLock::new();

fn parse_mode(mode: &str) -> (&str, bool, bool) {
    let mut actual = mode;
    let mut sfb = false;
    let mut debug = false;

    if mode == "update-efisp-with-superfastboot" {
        actual = "update-efisp";
        sfb = true;
    }
    if mode == "debug" {
        debug = true;
        actual = "skip-efisp";
    }
    if mode == "debug-with-superfastboot" {
        debug = true;
        actual = "update-efisp";
        sfb = true;
    }
    (actual, sfb, debug)
}

pub fn run_flash(mode: &str) -> Result<()> {
    set_path_env();
    ensure_runtime();
    crate::logging::init_log(&crate::util::log_file());

    if !acquire_lock() {
        write_log("Task is already running");
        bail!("busy");
    }

    CTRLC_SET.get_or_init(|| {
        let _ = ctrlc::set_handler(|| {
            release_lock();
            std::process::exit(1);
        });
    });

    let pid = std::process::id();
    let _ = fs::write(crate::util::pid_file(), pid.to_string());

    write_log("Task started");

    let cs = detect_current_slot().ok_or_else(|| anyhow::anyhow!("slot detection failed"))?;
    let ts = other_slot(&cs).ok_or_else(|| anyhow::anyhow!("target slot detection failed"))?;
    write_state("running", &format!("Flashing to slot {}", ts));

    let (actual, sfb, debug) = parse_mode(mode);

    if debug {
        let abl = partition_path("abl", &ts);
        if actual == "update-efisp" {
            match patch_efisp(&abl, sfb, true) {
                Ok(0) => write_state("success", "Debug done"),
                Ok(_) => write_state("error", "Debug error"),
                Err(e) => {
                    write_state("error", &format!("Debug error: {}", e));
                }
            }
        } else {
            let mut cmd = crate::util::cmd_tool("extractfv");
            cmd.args(["-o", runtime_dir().to_str().unwrap_or("."), "-v", &abl]);
            let status = cmd.output()?;
            write_log(&format!(
                "extractfv: {}",
                String::from_utf8_lossy(&status.stdout).trim()
            ));
            if runtime_dir().join("LinuxLoader.efi").exists() {
                write_state("success", "ABL extracted");
            } else {
                write_state("error", "ABL extract failed");
            }
        }
        release_lock();
        return Ok(());
    }

    let mut efisp_fail = 0;
    let abl = partition_path("abl", &ts);

    if actual == "update-efisp" {
        match patch_efisp(&abl, sfb, false) {
            Ok(2) => {
                write_state("success", "Skipped BL flash (GBL vuln)");
                release_lock();
                return Ok(());
            }
            Ok(1) => {
                efisp_fail = 1;
                write_state("running", "efisp failed, continue BL flash");
            }
            Ok(0) => {}
            Ok(_) => {}
            Err(e) => {
                write_state("error", &format!("{}", e));
                release_lock();
                return Ok(());
            }
        }
    } else {
        match detect_gbl_vulnerability(&abl) {
            Ok(0) => {
                write_state("success", "Skipped BL flash (GBL vuln)");
                release_lock();
                return Ok(());
            }
            Ok(_) => {}
            Err(e) => {
                write_log(&format!("Vuln check failed: {}", e));
            }
        }
    }

    for part in &["abl"] {
        let dst = partition_path(part, &ts);
        let src = partition_path(part, &cs);

        let status = crate::util::cmd_system("blockdev")
            .args(["--setrw", &dst])
            .status()?;
        if !status.success() {
            write_state("error", "setrw failed");
            release_lock();
            bail!("setrw failed");
        }

        let status = crate::util::cmd_system("dd")
            .args([
                &format!("if={}", src),
                &format!("of={}", dst),
                "bs=4M",
                "conv=fsync",
            ])
            .status()?;
        if !status.success() {
            write_state("error", &format!("Flash {} failed", part));
            release_lock();
            bail!("flash failed");
        }
        let _ = crate::util::cmd_system("sync").status();
        write_log(&format!("Flash {} -> {} done", part, dst));
    }

    if efisp_fail == 1 {
        write_state("warning", "BL done, efisp failed");
    } else if actual == "update-efisp" {
        write_state("success", "All done (with efisp)");
    } else {
        write_state("success", "All done (no efisp)");
    }

    release_lock();
    Ok(())
}

pub fn start_flash(mode: &str) {
    ensure_runtime();

    if current_pid().is_some() {
        println!("{}", json!({"already_running": true}));
        return;
    }

    let exe = match std::env::current_exe() {
        Ok(e) => e,
        Err(e) => {
            println!("{}", json!({"started": false, "error": e.to_string()}));
            return;
        }
    };

    match Command::new(&exe)
        .arg("flash")
        .arg(mode)
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
    
    {
        Ok(_) => {
            std::thread::sleep(std::time::Duration::from_secs(1));
            if current_pid().is_some() {
                println!("{}", json!({"started": true}));
            } else {
                let st = crate::status::get_status().state;
                if !st.is_empty() && st != "idle" {
                    println!("{}", json!({"finished": st}));
                } else {
                    println!("{}", json!({"started": false}));
                }
            }
        }
        Err(e) => {
            println!("{}", json!({"started": false, "error": e.to_string()}));
        }
    }
}
