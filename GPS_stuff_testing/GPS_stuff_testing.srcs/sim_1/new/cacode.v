`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/26/2023 03:51:35 PM
// Design Name: 
// Module Name: cacode
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


module cacode(
    input wire          rst,
    input wire          clk,
    input wire [4:0]    sv_sel,
    output wire         out,
    output wire         epoch
    );
        
    reg [3:0] T0 [31:0];
    reg [3:0] T1 [31:0];
    reg [10:1] g1, g2;
    
    initial begin
        T0[0] = 2;
        T0[1] = 3;
        T0[2] = 4;
        T0[3] = 5;
        T0[4] = 1;
        T0[5] = 2;
        T0[6] = 1;
        T0[7] = 2;
        T0[8] = 3;
        T0[9] = 2;
        T0[10] = 3;
        T0[11] = 5;
        T0[12] = 6;
        T0[13] = 7;
        T0[14] = 8;
        T0[15] = 9;
        T0[16] = 1;
        T0[17] = 2;
        T0[18] = 3;
        T0[19] = 4;
        T0[20] = 5;
        T0[21] = 6;
        T0[22] = 1;
        T0[23] = 4;
        T0[24] = 5;
        T0[25] = 6;
        T0[26] = 7;
        T0[27] = 8;
        T0[28] = 1;
        T0[29] = 2;
        T0[30] = 3;
        T0[31] = 4;

        T1[0] =  6;
        T1[1] =  7;
        T1[2] =  8;
        T1[3] =  9;
        T1[4] =  9;
        T1[5] =  10;
        T1[6] =  8;
        T1[7] =  9;
        T1[8] =  10;
        T1[9] =  3;
        T1[10] = 4;
        T1[11] = 6;
        T1[12] = 7;
        T1[13] = 8;
        T1[14] = 9;
        T1[15] = 10;
        T1[16] = 4;
        T1[17] = 5;
        T1[18] = 6;
        T1[19] = 7;
        T1[20] = 8;
        T1[21] = 9;
        T1[22] = 3;
        T1[23] = 6;
        T1[24] = 7;
        T1[25] = 8;
        T1[26] = 9;
        T1[27] = 10;
        T1[28] = 6;
        T1[29] = 7;
        T1[30] = 8;
        T1[31] = 9;
        
        g1 <= 10'b1111111111;
        g2 <= 10'b1111111111;
    end
    
    always @ (posedge clk) begin
        if (rst) begin
            g1 <= 10'b1111111111;
            g2 <= 10'b1111111111;
        end else begin
            g1[10:1] <= {g1[9:1], g1[3] ^ g1[10]};
            g2[10:1] <= {g2[9:1], g2[2] ^ g2[3] ^ g2[6] ^ g2[8] ^ g2[9] ^ g2[10]};
        end
    end

    assign out = g1[10] ^ g2[T0[sv_sel]] ^ g2[T1[sv_sel]];
    assign epoch = &g1;
    
endmodule
