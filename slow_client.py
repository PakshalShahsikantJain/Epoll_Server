# slow_client.py
import socket

N = 20_000_000
s = socket.create_connection(("127.0.0.1", 9000))
s.sendall(b"x" * N)            # send everything first, read nothing yet
s.shutdown(socket.SHUT_WR)     # tell the server we're done sending

total = 0
while True:
    chunk = s.recv(65536)
    if not chunk:
        break
    total += len(chunk)
print("sent", N, "received", total)