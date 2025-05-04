import socket

def get_ip_address():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))  # Connects to Google's public DNS server
    ip_address = s.getsockname()[0]
    s.close()
    return ip_address

ip = get_ip_address()
print("Your Replit server IP address is:", ip)
