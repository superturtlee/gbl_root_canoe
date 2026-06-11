use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

pub fn moddir() -> PathBuf {
    if let Ok(d) = env::var("MODDIR") {
        return PathBuf::from(d);
    }
    let exe = env::current_exe().unwrap_or_default();
    exe.parent()
        .and_then(|p| p.parent())
        .map(PathBuf::from)
        .unwrap_or_default()
}

pub fn runtime_dir() -> PathBuf {
    moddir().join("tmp")
}

pub fn bin_dir() -> PathBuf {
    moddir().join("bin")
}

pub fn lock_dir() -> PathBuf {
    runtime_dir().join("flash.lock")
}

pub fn pid_file() -> PathBuf {
    runtime_dir().join("flash.pid")
}

pub fn state_file() -> PathBuf {
    runtime_dir().join("state")
}

pub fn message_file() -> PathBuf {
    runtime_dir().join("message")
}

pub fn updated_file() -> PathBuf {
    runtime_dir().join("updated")
}

pub fn log_file() -> PathBuf {
    runtime_dir().join("flash.log")
}

pub fn by_name_dir() -> &'static str {
    "/dev/block/by-name"
}


pub fn detect_current_slot() -> Option<String> {
    let out = Command::new("getprop")
        .arg("ro.boot.slot_suffix")
        .output()
        .ok()?;
    let s = String::from_utf8_lossy(&out.stdout).trim().to_string();
    match s.as_str() {
        "_a" | "_b" => Some(s),
        _ => None,
    }
}

pub fn other_slot(slot: &str) -> Option<&'static str> {
    match slot {
        "_a" => Some("_b"),
        "_b" => Some("_a"),
        _ => None,
    }
}

pub fn partition_path(name: &str, slot: &str) -> String {
    format!("{}/{}{}", by_name_dir(), name, slot)
}

pub fn current_pid() -> Option<u32> {
    let pid_str = fs::read_to_string(pid_file()).ok()?;
    let pid: u32 = pid_str.trim().parse().ok()?;
    let proc_path = format!("/proc/{}", pid);
    if Path::new(&proc_path).exists() {
        Some(pid)
    } else {
        let _ = fs::remove_file(pid_file());
        None
    }
}

pub fn acquire_lock() -> bool {
    fs::create_dir(lock_dir()).is_ok()
}

pub fn release_lock() {
    let _ = fs::remove_dir_all(lock_dir());
    let _ = fs::remove_file(pid_file());
}

pub fn ensure_runtime() {
    let rt = runtime_dir();
    let _ = fs::create_dir_all(&rt);
    let lf = log_file();
    if !lf.exists() {
        let _ = fs::write(&lf, "");
    }
    let sf = state_file();
    if !sf.exists() {
        let _ = fs::write(&sf, "idle");
    }
    let mf = message_file();
    if !mf.exists() {
        let _ = fs::write(&mf, "Waiting");
    }
    let uf = updated_file();
    if !uf.exists() {
        let _ = fs::write(&uf, crate::logging::timestamp());
    }
}

pub fn set_path_env() {
    let cur = env::var("PATH").unwrap_or_default();
    env::set_var(
        "PATH",
        format!("/data/adb/ksu/bin:/system/bin:/system/xbin:{}", cur),
    );
}

pub fn cmd_tool(name: &str) -> Command {
    let mut c = Command::new(bin_dir().join(name));
    c.env("PATH", env::var("PATH").unwrap_or_default());
    c
}

pub fn cmd_system(name: &str) -> Command {
    let mut c = Command::new(name);
    c.env("PATH", env::var("PATH").unwrap_or_default());
    c.stdout(Stdio::piped());
    c.stderr(Stdio::piped());
    c
}
