"""
Custom OTA upload script for PlatformIO that uses ElegantOTA's HTTP endpoint.
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
    firmware_path = str(source[0])
    upload_port = env.GetProjectOption("upload_port", "hexagono.local")
    base_url = f"http://{upload_port}"

    if not os.path.exists(firmware_path):
        print(f"Error: firmware file not found: {firmware_path}")
        return 1

    file_size = os.path.getsize(firmware_path)

    # Calculate MD5 hash
    md5 = hashlib.md5()
    with open(firmware_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            md5.update(chunk)
    file_hash = md5.hexdigest()

    print(f"Firmware: {firmware_path} ({file_size} bytes, MD5: {file_hash})")
    print(f"Target:   {base_url}")

    try:
        # Step 1: Start OTA update
        start_url = f"{base_url}/ota/start?mode=firmware&hash={file_hash}"
        print(f"Starting OTA update...")
        resp = requests.get(start_url, timeout=10)
        if resp.status_code != 200:
            print(f"Error: /ota/start returned HTTP {resp.status_code}")
            print(resp.text)
            return 1

        # Step 2: Upload firmware binary
        print(f"Uploading firmware...")
        with open(firmware_path, "rb") as f:
            files = {"firmware": ("firmware.bin", f, "application/octet-stream")}
            resp = requests.post(f"{base_url}/ota/upload", files=files, timeout=120)

        if resp.status_code == 200:
            print("OTA upload successful! Device will reboot.")
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
