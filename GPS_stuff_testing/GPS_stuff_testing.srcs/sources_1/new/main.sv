`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 05/29/2024 07:27:15 PM
// Design Name: 
// Module Name: main
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


module main(
    );
    wire clk;
    wire rst;
    wire sample;
    wire [1:0] epoch;
    wire [108:1] tracked_outs [1:0];
    wire signed [18:1] ip [1:0];
    wire signed [18:1] qp [1:0];
    
    channel #(
        .SAMPLE_FREQ(69984000.0), 
        .IF_FREQ(9334875.0),
        .SV(5'd20),
        .LO_DOP(-2400.0),
        .CA_SHIFT(817.5)
    ) ch0 (
        clk,
        rst,
        sample,
        epoch[0],
        tracked_outs[0]
    );
    
    boc11_channel #(
        .SAMPLE_FREQ(69984000.0), 
        .IF_FREQ(9334875.0),
        .SV(5'd23),
        .LO_DOP(-246.0),
        .CA_SHIFT(2837.5)
    ) ch1 (
        clk,
        rst,
        sample,
        epoch[1],
        tracked_outs[1]
    );
    
    testbench tb(clk, rst, sample);
        
    int sav_num = 0;
    int fd;
    
    initial fd = $fopen("iq_sav.csv", "w");
    
    // Extract prompt I
    assign ip[0] = tracked_outs[0][72:55];
    assign ip[1] = tracked_outs[1][72:55];
    
    // Extract prompt Q
    assign qp[0] = tracked_outs[0][54:37];
    assign qp[1] = tracked_outs[1][54:37];
    
    always @(posedge epoch[1]) begin
        if(sav_num < 499) begin
            $fdisplay(fd, "%d,%d", ip[1], qp[1]);
            sav_num = sav_num + 1;
        end else begin
            $fclose(fd);
        end
    end
    
endmodule
