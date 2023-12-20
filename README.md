# PSP-HA-GPS
An SRAD GPS receiver project derived heavily from a similar project by [Andrew Holme](http://www.aholme.co.uk/GPS/Main.htm).

## Usage
The "GPS_stuff_testing" is a Vivado project meant for testing Verilog in the simulator. The "fpga_c_model" directory contains C code meant for simulating and testing the receiver in software. The fft_correlator.cpp program can be used for acquisition and the doppler and ca_shifts can then be entered manually in the other projects.
