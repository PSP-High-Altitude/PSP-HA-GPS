`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 05/29/2024 08:13:26 PM
// Design Name: 
// Module Name: loop_tracker
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


module loop_tracker(
    input wire [108:1] tracked_ins,
    input wire signed [63:0] ca_freq_integrator,
    input wire signed [63:0] lo_freq_integrator,
    output wire signed [63:0] new_ca_freq_integrator,
    output wire signed [63:0] new_lo_freq_integrator,
    output wire [31:0] new_ca_rate,
    output wire [31:0] new_lo_rate
    );
    
    wire signed [18:1] ie;
    wire signed [18:1] qe;
    wire signed [18:1] ip; 
    wire signed [18:1] qp;
    wire signed [18:1] il; 
    wire signed [18:1] ql;
    
    assign {ie, qe, ip, qp, il, ql} = tracked_ins;
    
    wire signed [63:0] power_early;
    wire signed [63:0] power_late;
    wire signed [63:0] new_ca_rate_long;
    wire signed [63:0] new_lo_rate_long;
    wire signed [63:0] code_phase_err;
    wire signed [63:0] carrier_phase_err;
    
    // Delay locked loop error calculations
    assign power_early = (ie**2)+(qe**2);
    assign power_late = (il**2)+(ql**2);
    assign code_phase_err = power_early - power_late;
    
    // CA code tracking loop
    // 10M sample rate: assign new_ca_freq_integrator = ca_freq_integrator + (code_phase_err <<< 11);
    // 10M sample rate: assign new_ca_rate_long = ca_freq_integrator + (code_phase_err <<< 23);
    // 5M sample rate: assign new_ca_freq_integrator = ca_freq_integrator + (code_phase_err <<< 11);
    // 5M sample rate: assign new_ca_rate_long = ca_freq_integrator + (code_phase_err <<< 23);
    assign new_ca_freq_integrator = ca_freq_integrator + (code_phase_err <<< 9);
    assign new_ca_rate_long = ca_freq_integrator + (code_phase_err <<< 19);
    assign new_ca_rate = new_ca_rate_long[63:32];
    
    // Carrier tracking loop
    assign carrier_phase_err = ip*qp;
    // 10M sample rate: assign new_lo_freq_integrator = lo_freq_integrator + (carrier_phase_err <<< 20);
    // 10M sample rate: assign new_lo_rate_long = lo_freq_integrator + (carrier_phase_err <<< 27);
    // 5M sample rate: assign new_lo_freq_integrator = lo_freq_integrator + (carrier_phase_err <<< 21);
    // 5M sample rate: assign new_lo_rate_long = lo_freq_integrator + (carrier_phase_err <<< 28);
    assign new_lo_freq_integrator = lo_freq_integrator + (carrier_phase_err <<< 19);
    assign new_lo_rate_long = lo_freq_integrator + (carrier_phase_err <<< 24);
    assign new_lo_rate = new_lo_rate_long[63:32];
    
endmodule
