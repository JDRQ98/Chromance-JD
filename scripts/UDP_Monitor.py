import socket
import datetime

UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 8888

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening for UDP data on {UDP_IP}:{UDP_PORT}")

while True:
    data, addr = sock.recvfrom(1024)  # Buffer size of 1024 bytes
    try:
        message = data.decode('utf-8').strip() # Decode the data and remove trailing whitespace
    except UnicodeDecodeError:
        message = f"<binary data: {len(data)} bytes>" # Handle binary data cases

    now = datetime.datetime.now() #Get current timestamp
    timestamp = now.strftime("%H:%M:%S.%f")[:-3] #Convert to string, add milliseconds, then cut off microseconds

    print(f"[{timestamp}] {message}")