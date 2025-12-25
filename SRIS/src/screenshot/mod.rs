use screenshots::Screen;

pub fn take_screenshot()
-> Option<Vec<screenshots::image::ImageBuffer<screenshots::image::Rgba<u8>, Vec<u8>>>> {
    let screens = Screen::all();

    if screens.is_err() {
        return None;
    }

    let screens = screens.unwrap();
    let mut images = Vec::new();

    for screen in screens {
        let image = screen.capture();

        if let Ok(img) = image {
            images.push(img);
        }
    }

    if images.is_empty() {
        return None;
    }

    Some(images)
}