use std::{fs, path::PathBuf};
use sysinfo::System;

use super::config;

pub fn kill_browser_processes() {
    let browsers = config::get_browser_list();
    let mut sys = System::new_all();
    sys.refresh_all();

    for browser in browsers {
        sys.processes().values().for_each(|process| {
            if process.name().eq_ignore_ascii_case(browser.process) {
                process.kill();
            }
        });
    }
}

pub fn discover_profiles(browser_path: &PathBuf) -> Vec<String> {
    let mut profiles = vec![String::new()];

    if let Ok(entries) = fs::read_dir(browser_path) {
        for entry in entries.flatten() {
            let file_name = entry.file_name().into_string().unwrap_or_default();
            if file_name.starts_with("Profile ") || file_name == "Default" {
                profiles.push(file_name);
            }
        }
    }

    if profiles.len() == 1 {
        config::default_profiles()
    } else {
        profiles
    }
}

pub fn copy_directory(src: &PathBuf, dst: &PathBuf) -> std::io::Result<()> {
    fs::create_dir_all(dst)?;

    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let file_type = entry.file_type()?;
        let src_path = entry.path();
        let dst_path = dst.join(entry.file_name());

        if file_type.is_dir() {
            copy_directory(&src_path, &dst_path)?;
        } else {
            fs::copy(&src_path, &dst_path)?;
        }
    }

    Ok(())
}
