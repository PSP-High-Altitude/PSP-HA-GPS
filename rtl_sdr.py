from rtlsdr import RtlSdr

sdr = RtlSdr()

fc = 1e6

# configure device
sdr.set_bias_tee(True)
sdr.sample_rate = 3.2e6     # Hz
sdr.center_freq = 1575.42e6 - fc # Hz
print(sdr.get_gains())
sdr.set_gain(328)
sdr.bandwidth = sdr.sample_rate

ms_to_record = 1000
samples_to_record = int(ms_to_record * sdr.sample_rate / 1000)
print("Samples to record: ", samples_to_record)

# read samples
samples = sdr.read_samples(samples_to_record)
samples_i = [x.real > 0 for x in samples]
print("Samples read: ", len(samples_i))

#lo_sin = [0, 0, 1, 1]
#
#lo_phase = int(0)
#lo_rate = int(fc/sdr.sample_rate*(2**32))
#for i in range(len(samples_i)):
#    lo_phase = (lo_phase + lo_rate) & 0xFFFFFFFF
#    samples_i[i] = int(samples_i[i]) ^ lo_sin[(lo_phase >> 30) & 0x3]

with open('rtl_samples.bin', 'wb') as f:
    by = 0
    for i in range(len(samples_i)):
        by = by | (int(samples_i[i]) << (i % 8))
        if i % 8 == 7:
            f.write(int(by).to_bytes(1, byteorder='big'))
            by = 0
    if i % 8 != 7:
        f.write(by)
    f.close()