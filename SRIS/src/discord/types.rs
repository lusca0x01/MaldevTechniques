use std::path::PathBuf;

#[derive(Debug, Clone)]
pub struct AppInfo {
    pub name: &'static str,
    pub path: PathBuf,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash, serde::Serialize)]
pub struct DiscordToken {
    pub token: String,
    pub platform: String,
}
