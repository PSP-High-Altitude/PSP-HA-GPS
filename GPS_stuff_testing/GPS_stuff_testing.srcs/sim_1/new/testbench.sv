//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 09/01/2023 04:34:29 PM
// Design Name: 
// Module Name: testbench
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////
`timescale 1ps / 1ps

module testbench(
        output reg clk,
        output reg sample
    );

    specparam PERIOD = 500000000 / 10000;
    
    initial begin
        clk = 0;
    end
    
    always #PERIOD clk <= ~clk;
    
    reg file_bit = 0; 
    reg [2:0] file_buf_idx = 0;
    reg [7:0] file_buf = 0;
    reg [2:0] save_idx = 0;
    
    integer file;
    
    initial begin
        file = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/gps.samples.1bit.I.fs10000.if2716.bin", "rb");
    end
    
    always @ (posedge clk) begin
        if(file_buf_idx == 0) begin
            $fread(file_buf, file);
        end
        
        sample = file_buf[file_buf_idx];
        file_buf_idx += 1;
    end
endmodule
