import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

fil = open('C:/Users/griff/Documents/PSP-HA-GPS/fpga_c_model/log/iq-gps-5.csv', 'r')
data = pd.read_csv(fil, sep=',', header=None)
fil.close()

edge_hist = np.zeros(20)

edge = 0
for i in range(1, 1000):
    ip = data[0][i] > 0
    ip_last = data[0][i-1] > 0

    if ip != ip_last:
        edge_hist[edge] += 1

    edge = (edge + 1) % 20

print(edge_hist)
plt.bar(np.arange(len(edge_hist)),edge_hist)
plt.xticks(range(0,20))
plt.show()