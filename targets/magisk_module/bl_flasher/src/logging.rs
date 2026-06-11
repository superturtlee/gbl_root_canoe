// logging.rs — single source of truth for everything we write to disk.
//
// Previously there were three separate systems duct-taped together:
//   - write_log()  → eprintln!, which only works because the child process
//                    has stderr redirected to flash.log at spawn time.
//                    Direct calls from the parent process silently disappear.
//   - write_state() → three separate fs::write()s that are not atomic.
//                     A reader can catch us mid-update and see stale state.
//   - timestamp()  → forks a `date` process every single call.
//
// This module fixes all three problems.

use std::fs::{self, File, OpenOptions};
use std::io::Write;
use std::path::PathBuf;
use std::sync::{Mutex, OnceLock};
use std::time::{SystemTime, UNIX_EPOCH};

use serde::Serialize;

use crate::util::{
    current_pid, detect_current_slot, ensure_runtime, message_file, other_slot,
    state_file, updated_file,
};

// ---------------------------------------------------------------------------
// Timestamp — no more forking `date` on every log line.
// ---------------------------------------------------------------------------

pub fn timestamp() -> String {
    // We need a human-readable string for the log; pulling in chrono for this
    // feels like overkill, so we do the math ourselves.  UNIX epoch -> ymdhms.
    let secs = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);

    // Quick-and-dirty calendar math; good enough for a flash log.
    let s = secs % 60;
    let m = (secs / 60) % 60;
    let h = (secs / 3600) % 24;
    let days = secs / 86400; // days since 1970-01-01

    // Gregorian calendar unrolling — handles leap years correctly until 2100.
    let (y, doy) = {
        let mut y = 1970u64;
        let mut d = days;
        loop {
            let leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
            let ydays = if leap { 366 } else { 365 };
            if d < ydays {
                break (y, d);
            }
            d -= ydays;
            y += 1;
        }
        (y, d)
    };
    let month_days = {
        let leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
        [31u64, if leap { 29 } else { 28 }, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
    };
    let mut mo = 1u64;
    let mut rem = doy;
    for &md in &month_days {
        if rem < md {
            break;
        }
        rem -= md;
        mo += 1;
    }
    let day = rem + 1;

    format!("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", y, mo, day, h, m, s)
}

// ---------------------------------------------------------------------------
// Log writer — opens the file once per process and keeps the handle alive.
// ---------------------------------------------------------------------------

// A Mutex<File> stored in a OnceLock gives us a single open handle for the
// life of the process.  That's both faster and simpler than reopening on
// every write_log() call.
static LOG_WRITER: OnceLock<Mutex<File>> = OnceLock::new();

/// Call this once at the start of run_flash(), before any other logging.
/// After this, write_log() goes to the file *and* stderr simultaneously.
pub fn init_log(path: &PathBuf) {
    // Truncate on open so we start with a clean log for each flash run.
    // The webui always fetches the full log anyway.
    let file = OpenOptions::new()
        .create(true)
        .write(true)
        .truncate(true)
        .open(path)
        .expect("failed to open log file");

    // If init is called twice (shouldn't happen, but just in case), the
    // second call is silently ignored by OnceLock.
    let _ = LOG_WRITER.set(Mutex::new(file));
}

/// Append a timestamped line to the log file (if init_log was called) and
/// to stderr (always, so `logcat` / adb shell still shows output).
pub fn write_log(msg: &str) {
    let line = format!("[{}] {}\n", timestamp(), msg);

    if let Some(writer) = LOG_WRITER.get() {
        // Unwrap on poison: if the mutex is poisoned, something has gone
        // badly wrong elsewhere and we might as well surface it.
        let mut f = writer.lock().unwrap();
        let _ = f.write_all(line.as_bytes());
        // Flush every line so the webui tail always sees complete entries.
        let _ = f.flush();
    }

    // stderr goes to logcat / the terminal regardless of whether a file is open.
    eprint!("{}", line);
}

// ---------------------------------------------------------------------------
// State file — atomic write so readers never see partial state.
// ---------------------------------------------------------------------------

// The old code wrote three separate files (state, message, updated) with
// three separate fs::write() calls.  A concurrent reader could see any
// combination of old/new values across those three files.
//
// We now write a single JSON file via write-then-rename.  On Linux (and
// Android's ext4/f2fs), rename(2) is atomic, so the reader always sees
// either the old complete state or the new complete state.

#[derive(Serialize)]
struct StateFile<'a> {
    state: &'a str,
    message: &'a str,
    updated_at: String,
}

pub fn write_state(state: &str, message: &str) {
    ensure_runtime();

    let payload = StateFile {
        state,
        message,
        updated_at: timestamp(),
    };

    // Write to a temp file first, then rename into place.
    let final_path = state_file();
    let tmp_path = final_path.with_extension("tmp");

    match serde_json::to_string(&payload) {
        Ok(json) => {
            // If the tmp write fails, leave the old state file untouched.
            if fs::write(&tmp_path, &json).is_ok() {
                // rename is the atomic step; ignore errors (e.g. read-only fs).
                let _ = fs::rename(&tmp_path, &final_path);
            } else {
                write_log(&format!("write_state: failed to write tmp file"));
            }
        }
        Err(e) => {
            // Shouldn't happen with static strings, but let's not panic.
            write_log(&format!("write_state: json serialization failed: {}", e));
        }
    }

    // Keep the old separate message / updated files around so the legacy
    // status reader in get_status() (which reads three separate files) still
    // works while we're mid-migration.  Once the callers are all updated to
    // read the new single-file format, these can be removed.
    let _ = fs::write(message_file(), message);
    let _ = fs::write(updated_file(), timestamp());
}

// ---------------------------------------------------------------------------
// Status struct — same API as before, no changes needed in callers.
// ---------------------------------------------------------------------------

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

    // Try to read from the new single-file format first; fall back to the
    // old three-file layout so we don't break anything during the transition.
    let (state, message, updated_at) = read_state_json()
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

/// Parse the new single-JSON state file.  Returns None if the file doesn't
/// exist yet or can't be parsed (e.g. device was on the old format).
fn read_state_json() -> Option<(String, String, String)> {
    #[derive(serde::Deserialize)]
    struct S {
        state: String,
        message: String,
        updated_at: String,
    }
    let raw = fs::read_to_string(state_file()).ok()?;
    let s: S = serde_json::from_str(&raw).ok()?;
    Some((s.state, s.message, s.updated_at))
}

pub fn print_status() -> anyhow::Result<()> {
    let s = get_status();
    let json = serde_json::to_string(&s)?;
    println!("{}", json);
    Ok(())
}
