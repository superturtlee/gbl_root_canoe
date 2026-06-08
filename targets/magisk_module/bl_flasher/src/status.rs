use anyhow::Result;
use serde::Serialize;
use std::fs;
use std::io::Write;

use crate::util::{
    current_pid, detect_current_slot, ensure_runtime, log_file, message_file, other_slot,
    state_file, timestamp, updated_file,
};

#[derive(Serialize)]
pub struct Status {
    pub current_slot: String,
    pub target_slot: String,
    pub running: bool,
    pub pid: Option<u32>,
    pub state: String,
    pub message: String,
    pub updated_at: String,
}

fn read_line(path: &std::path::Path) -> String {
    fs::read_to_string(path)
        .unwrap_or_default()
        .lines()
        .next()
        .unwrap_or("")
        .to_string()
}

pub fn get_status() -> Status {
    ensure_runtime();
    let cs = detect_current_slot().unwrap_or_else(|| "-".into());
    let ts = other_slot(&cs).unwrap_or("-").to_string();
    let pid = current_pid();
    let running = pid.is_some();
    let state = read_line(&state_file());
    let message = read_line(&message_file());
    let updated_at = read_line(&updated_file());

    Status {
        current_slot: cs,
        target_slot: ts,
        running,
        pid,
        state,
        message,
        updated_at,
    }
}

pub fn print_status() -> Result<()> {
    let s = get_status();
    let json = serde_json::to_string(&s)?;
    println!("{}", json);
    Ok(())
}

pub fn write_state(state: &str, message: &str) {
    ensure_runtime();
    let _ = fs::write(state_file(), state);
    let _ = fs::write(message_file(), message);
    let _ = fs::write(updated_file(), timestamp());
}

pub fn write_log(msg: &str) {
    ensure_runtime();
    let line = format!("[{}] {}\n", timestamp(), msg);
    if let Ok(mut f) = fs::OpenOptions::new().append(true).create(true).open(log_file()) {
        let _ = f.write_all(line.as_bytes());
    }
}
