import numpy as np
import matplotlib.pyplot as plt
from control import tf, bode

# Define constants
# rssi = 500  # amplitude
# f1 = 10e6   # 10 MHz
# f2 = 1e3    # 1 KHz

rssi = 1000  # amplitude
f1 = 69.984e6   # 69.984 MHz
f2 = 1e3    # 0.25 KHz

# Calculate parameters
kPD = rssi**2
kNCO = 2 * np.pi * (f1 / f2)

kI = 2**(16 - 64)
kP = 2**(24 - 64)

# Define the z-transform with a sample time
z = tf([1, 0], [1], 1 / f2)  # Z-transform variable
kNCO_tf = kNCO / (z - 1)  # Represent kNCO as a transfer function
G = kPD * kNCO_tf * (kP + kI / (z - 1))  # Full transfer function G

# Bode plot
plt.figure()
magnitude, phase, omega = bode(-G, dB=True, Hz=True, omega_limits=(1e-3, 1000), omega_num=500)
plt.show()
