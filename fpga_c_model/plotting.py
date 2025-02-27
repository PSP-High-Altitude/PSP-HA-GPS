import os
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import sys

ip = []
qp = []
iq_data = [[], []]
time = []
time_data = [[], []]

def plot_iq(fname):
    with open(fname, 'r') as f:
        fdata = f.readlines()
        for fline in fdata:
            ip.append(int(fline.split(',')[0]))
            qp.append(int(fline.split(',')[1]))
        f.close()

    # animation
    fig, ax = plt.subplots()
    global scatter
    scatter = ax.scatter([], [])
    ax.grid(True, which='both')

    # set the x-spine (see below for more info on `set_position`)
    ax.spines['left'].set_position('zero')

    # turn off the right spine/ticks
    ax.spines['right'].set_color('none')
    ax.yaxis.tick_left()

    # set the y-spine
    ax.spines['bottom'].set_position('zero')

    # turn off the top spine/ticks
    ax.spines['top'].set_color('none')
    ax.xaxis.tick_bottom()
    plt.xlabel('I')
    plt.ylabel('Q')
    plt.title('I vs Q')
    ax.set_xlim(-6000, 6000)
    ax.set_ylim(-6000, 6000)
    ani = animation.FuncAnimation(fig, animate_iq, init_func=init_iq, interval=1, frames=len(ip), blit=True, repeat=False)
    plt.show()

def init_iq():
    scatter.set_offsets(np.empty((0, 2)))
    return scatter,

def animate_iq(i):
    iq_data[0].append(ip[i])
    iq_data[1].append(qp[i])
    if len(iq_data[0]) > 200:
        iq_data[0].remove(iq_data[0][0])
        iq_data[1].remove(iq_data[1][0])
    scatter.set_offsets(np.c_[iq_data[0], iq_data[1]])
    return scatter,
    
def plot_time(fname):
    with open(fname, 'r') as f:
        fdata = f.readlines()
        for fline in fdata:
            time.append(float(fline.split(',')[5]))
        f.close()

    # animation
    global ax, fig
    fig, ax = plt.subplots()
    global line
    line, = ax.plot([], [])
    ax.grid(True, which='both')

    ## set the x-spine (see below for more info on `set_position`)
    #ax.spines['left'].set_position('zero')
#
    ## turn off the right spine/ticks
    #ax.spines['right'].set_color('none')
    #ax.yaxis.tick_left()
#
    ## set the y-spine
    #ax.spines['bottom'].set_position('zero')
#
    ## turn off the top spine/ticks
    #ax.spines['top'].set_color('none')
    #ax.xaxis.tick_bottom()
    plt.xlabel('index')
    plt.ylabel('time')
    plt.title('time vs. index')
    #ax.set_xlim(0, 200)
    #ax.set_ylim(-6000, 6000)
    ani = animation.FuncAnimation(fig, animate_time, init_func=init_time, interval=1, frames=len(time), blit=True, repeat=False)
    plt.show()

def init_time():
    line.set_data([], [])
    return line,

def animate_time(i):
    time_data[0].append(i)
    time_data[1].append(time[i])
    if len(time_data[0]) > 200:
        time_data[0].remove(time_data[0][0])
        time_data[1].remove(time_data[1][0])
    line.set_data(time_data[0], time_data[1])
    ax.set_xlim(np.min(time_data[0]), max(time_data[0]))
    ax.set_ylim(np.min(time_data[1]), max(time_data[1]))
    if i % 100 == 0:
        fig.canvas.draw_idle()
    return line,

def plot_filter(fname):
    code_err = []
    code_int = []
    code_rate = []

    carrier_err = []
    carrier_int = []
    carrier_rate = []

    with open(fname, 'r') as f:
        fdata = f.readlines()
        for fline in fdata:
            line_split = fline.split(',')
            code_err.append(float(line_split[0]))
            code_int.append(float(line_split[1]))
            code_rate.append(float(line_split[2]))
            carrier_err.append(float(line_split[3]))
            carrier_int.append(float(line_split[4]))
            carrier_rate.append(float(line_split[5]))
        f.close()

    fig, ax = plt.subplots()
    #ax1 = ax.twinx()
    ax2 = ax.twinx()
    ax.plot(code_err, label='code_err', color="red")
    #ax1.plot(code_int, label='code_int', color="green")
    ax2.plot(code_rate, label='code_rate', color="blue")
    fig.legend(loc='upper right')

    fig, ax = plt.subplots()
    #ax1 = ax.twinx()
    ax2 = ax.twinx()
    ax.plot(carrier_err, label='carrier_err', color="red")
    #ax1.plot(carrier_int, label='carrier_int', color="green")
    ax2.plot(carrier_rate, label='carrier_rate', color="blue")
    fig.legend(loc='upper right')

    plt.show()

def plot_power(fname):
    cn0 = []

    with open(fname, 'r') as f:
        fdata = f.readlines()
        for fline in fdata:
            line_split = fline.split(',')
            cn0.append(float(line_split[0]))
        f.close()

    plt.plot(cn0)

    plt.show()

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: python plotting.py <data_type> <filename>')
        sys.exit(1)
    if(sys.argv[1] == 'iq'):
        plot_iq(sys.argv[2])
    elif(sys.argv[1] == 'time'):
        plot_time(sys.argv[2])
    elif(sys.argv[1] == 'filter'):
        plot_filter(sys.argv[2])
    elif(sys.argv[1] == 'power'):
        plot_power(sys.argv[2])