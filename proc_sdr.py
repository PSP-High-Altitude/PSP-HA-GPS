from rtlsdr import RtlSdr
import numpy as np

# read samples
with open('GPS-L1-2022-03-27.sigmf-data', 'rb') as f:
    samples = np.fromfile(f, dtype=np.int16, count=-1)
    f.close()
samples_i = samples[::2] < 0
print("Samples read: ", len(samples_i))

fc = 1.2e6
fs = 4e6

lo_sin = [0, 0, 1, 1]

lo_phase = int(0)
lo_rate = int(fc/fs*(2**32))
for i in range(len(samples_i)):
    lo_phase = (lo_phase + lo_rate) & 0xFFFFFFFF
    samples_i[i] = int(samples_i[i]) ^ lo_sin[(lo_phase >> 30) & 0x3]

with open('GPS-L1-2022-03-27.bin', 'wb') as f:
    by = 0
    for i in range(len(samples_i)):
        by = by | (int(samples_i[i]) << (i % 8))
        if i % 8 == 7:
            f.write(int(by).to_bytes(1, byteorder='big'))
            by = 0
    if i % 8 != 7:
        f.write(by)
    f.close()