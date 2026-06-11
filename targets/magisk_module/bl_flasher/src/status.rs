use anyhow::Result;
use serde::Serialize;
use std::fs;

use crate::util::{
    current_pid, detect_current_slot, ensure_runtime, message_file, other_slot,
    state_file, updated_file,
};

// re-export from logging so call sites don't need to change their use paths
pub use crate::logging::{write_log, write_state};

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

    let (state, message, updated_at) = crate::logging::read_state_json()
        .unwrap_or_else(|| {
            (
                read_line(&state_file()),
                read_line(&message_file()),
                read_line(&updated_file()),
            )
        });

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