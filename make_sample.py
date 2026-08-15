import struct
def framed(p): return struct.pack('>H', len(p)) + p
A = b'A'+struct.pack('>H',1)+struct.pack('>H',0)+(12345).to_bytes(6,'big')+struct.pack('>Q',1001)+b'B'+struct.pack('>I',100)+b'AAPL    '+struct.pack('>I',1502500)
E = b'E'+struct.pack('>H',1)+struct.pack('>H',0)+(12346).to_bytes(6,'big')+struct.pack('>Q',1001)+struct.pack('>I',50)+struct.pack('>Q',9001)
D = b'D'+struct.pack('>H',1)+struct.pack('>H',0)+(12347).to_bytes(6,'big')+struct.pack('>Q',1001)
assert (len(A),len(E),len(D))==(36,31,19)
open('data/sample.itch','wb').write(framed(A)+framed(E)+framed(D)+framed(A))
print("wrote data/sample.itch")