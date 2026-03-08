"""
Custom OTA upload script for PlatformIO that uses ElegantOTA's HTTP endpoint.
Handles both firmware and filesystem (LittleFS) uploads.
Two-step process: GET /ota/start, then POST /ota/upload.
"""
import hashlib
import os
import sys

try:
    import requests
except ImportError:
    env = DefaultEnvironment()
    env.Execute("$PYTHONEXE -m pip install requests")
    import requests

Import("env")


def upload_via_elegantota(source, target, env):
    file_path = str(source[0])
    upload_port = env.GetProjectOption("upload_port", "hexagono.local")
    base_url = f"http://{upload_port}"

    if not os.path.exists(file_path):
        print(f"Error: file not found: {file_path}")
        return 1

    # Detect firmware vs filesystem from the source filename
    filename = os.path.basename(file_path)
    if "littlefs" in filename or "spiffs" in filename:
        ota_mode = "fs"
        label = "filesystem"
    else:
        ota_mode = "firmware"
        label = "firmware"

    file_size = os.path.getsize(file_path)

    # Calculate MD5 hash
    md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            md5.update(chunk)
    file_hash = md5.hexdigest()

    print(f"Upload:   {label} ({filename}, {file_size} bytes, MD5: {file_hash})")
    print(f"Target:   {base_url}")

    try:
        # Step 1: Start OTA update
        start_url = f"{base_url}/ota/start?mode={ota_mode}&hash={file_hash}"
        print(f"Starting {label} OTA update...")
        resp = requests.get(start_url, timeout=10)
        if resp.status_code != 200:
            print(f"Error: /ota/start returned HTTP {resp.status_code}")
            print(resp.text)
            return 1

        # Step 2: Upload binary
        print(f"Uploading {label}...")
        with open(file_path, "rb") as f:
            files = {"firmware": (filename, f, "application/octet-stream")}
            resp = requests.post(f"{base_url}/ota/upload", files=files, timeout=120)

        if resp.status_code == 200:
            print(f"{label.capitalize()} OTA upload successful! Device will reboot.")
            return 0
        else:
            print(f"OTA upload failed: HTTP {resp.status_code}")
            print(resp.text)
            return 1

    except requests.exceptions.ConnectionError:
        print(f"Error: Could not connect to {base_url}")
        print("Make sure the device is powered on and connected to WiFi.")
        return 1
    except requests.exceptions.Timeout:
        print("Error: Upload timed out.")
        return 1
    except Exception as e:
        print(f"Error: {e}")
        return 1


env.Replace(UPLOADCMD=upload_via_elegantota)
env.Replace(UPLOADFSCMD=upload_via_elegantota)
