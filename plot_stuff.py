import matplotlib.pyplot as plt
import numpy as np
import pandas

iq_data = pandas.read_csv('iq.csv', header=None).to_numpy()
print(iq_data)

plt.scatter(iq_data[:, 0], iq_data[:, 1])
plt.show()