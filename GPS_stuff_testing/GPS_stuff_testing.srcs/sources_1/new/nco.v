`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/27/2023 05:04:09 PM
// Design Name: 
// Module Name: nco
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


module nco (
    input wire en,
    input wire clk,
    input wire[31:0] fcw,
    output wire full,
    output wire half
    );
    
    reg [31:0] pa = 0;
    
    always @ (posedge clk) begin
        if(en) begin
            pa = pa + fcw;
        end
    end
    
    assign full = pa[31];
    assign half = pa[30];
    
endmodule
