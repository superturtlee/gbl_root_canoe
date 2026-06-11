use std::env;
use std::fs;
use std::io::{BufRead, BufReader};
use std::process;

mod flash;
mod patch;
mod status;
mod util;
mod logging;

fn print_log(tail: bool) {
    let lf = crate::util::log_file();
    if !lf.exists() {
        println!();
        return;
    }
    let f = match fs::File::open(&lf) {
        Ok(f) => f,
        Err(_) => {
            println!();
            return;
        }
    };
    let reader = BufReader::new(f);
    let lines: Vec<String> = reader.lines().filter_map(|l| l.ok()).collect();

    if tail {
        let start = if lines.len() > 200 {
            lines.len() - 200
        } else {
            0
        };
        for line in &lines[start..] {
            println!("{}", line);
        }
    } else {
        for line in &lines {
            println!("{}", line);
        }
    }
}

fn clear_log() {
    if crate::util::current_pid().is_some() {
        println!("{}", serde_json::json!({"busy": true}));
        return;
    }
    let _ = fs::write(crate::util::log_file(), "");
    crate::status::write_state("idle", "Log cleared");
    println!("{}", serde_json::json!({"cleared": true}));
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        process::exit(1);
    }

    match args[1].as_str() {
        "status" => {
            if let Err(e) = crate::status::print_status() {
                eprintln!("Error: {}", e);
                process::exit(1);
            }
        }
        "flash" => {
            let mode = args.get(2).map(|s| s.as_str()).unwrap_or("skip-efisp");
            if let Err(e) = crate::flash::run_flash(mode) {
                eprintln!("Error: {}", e);
                process::exit(1);
            }
        }
        "start" => {
            let mode = args.get(2).map(|s| s.as_str()).unwrap_or("skip-efisp");
            crate::flash::start_flash(mode);
        }
        "log" => print_log(false),
        "tail" => print_log(true),
        "clear-log" => clear_log(),
        _ => process::exit(1),
    }
}
