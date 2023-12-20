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

module testbench();
    specparam PERIOD = 500000000 / 16368;
    specparam PERIOD_SAMPLE = 500000000 / 10000;

    reg clk = 0;
    reg sample_clk = 0;
    always #PERIOD clk <= ~clk;
    always #PERIOD_SAMPLE sample_clk <= ~sample_clk;
    main dut (
        clk,
        sample_clk
    );
endmodule
