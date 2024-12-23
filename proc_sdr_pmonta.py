from rtlsdr import RtlSdr
import numpy as np

# read samples
with open('gnss-20170427-L1.bin', 'rb') as f:
    samples = np.fromfile(f, dtype=np.int8, count=-1)
    f.close()
samples_i = samples[::2] < 0
print("Samples read: ", len(samples_i))

# Write as one bit I
with open('gnss-20170427-L1.3bit.I.bin', 'wb') as f:
    by = 0
    for i in range(len(samples_i)):
        by = by | (int(samples_i[i]) << (i % 8))
        if i % 8 == 7:
            f.write(int(by).to_bytes(1, byteorder='big'))
            by = 0
    if i % 8 != 7:
        f.write(by)
    f.close()