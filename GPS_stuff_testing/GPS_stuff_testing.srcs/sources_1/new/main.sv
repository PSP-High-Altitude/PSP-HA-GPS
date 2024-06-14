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
    wire sample;
    wire [1:0] epoch;
    wire [84:1] tracked_outs [1:0];
    reg [3:0] ch_to_track = 0;
    wire signed [14:1] ip [1:0];
    wire signed [14:1] qp [1:0];
    
    channel #(
        .SAMPLE_FREQ(69984000.0), 
        .IF_FREQ(9334875.0),
        .SV(5'd4),
        .LO_DOP(1350.0),
        .CA_SHIFT(969.4)
    ) ch0 (
        clk,
        sample,
        epoch[0],
        tracked_outs[0]
    );
    
    channel #(
        .SAMPLE_FREQ(69984000.0), 
        .IF_FREQ(9334875.0),
        .SV(5'd20),
        .LO_DOP(-2450.0),
        .CA_SHIFT(817.4)
    ) ch1 (
        clk,
        sample,
        epoch[1],
        tracked_outs[1]
    );
    
    testbench tb(clk, sample);
    
    reg signed [63:0] ca_freq_integrator;
    reg signed [63:0] lo_freq_integrator;
    wire signed [63:0] new_ca_freq_integrator;
    wire signed [63:0] new_lo_freq_integrator;
    wire [31:0] new_ca_rate;
    wire [31:0] new_lo_rate;
    
    // Retrieve the correct current channel rates and integrators
    always_comb begin
        case (ch_to_track)
            4'd0: begin
                ca_freq_integrator = ch0.ca_freq_integrator;
                lo_freq_integrator = ch0.lo_freq_integrator;
            end
            4'd1: begin
                ca_freq_integrator = ch1.ca_freq_integrator;
                lo_freq_integrator = ch1.lo_freq_integrator;
            end
            default: begin
                ca_freq_integrator = 0;
                lo_freq_integrator = 0;
            end
        endcase
    end
    
    // Computes the next rates and integrators
    loop_tracker trk(
        tracked_outs[ch_to_track], 
        ca_freq_integrator,
        lo_freq_integrator,
        new_ca_freq_integrator,
        new_lo_freq_integrator,
        new_ca_rate,
        new_lo_rate
    );
        
    // Set the new rates and integrators
    always_ff @(posedge (^epoch)) begin
        // Only go if the tracked_outs are valid
        if(|tracked_outs[ch_to_track]) begin
            case (ch_to_track)
                4'd0: begin
                    ch0.ca_freq_integrator = new_ca_freq_integrator;
                    ch0.lo_freq_integrator = new_lo_freq_integrator;
                    ch0.ca_rate = new_ca_rate;
                    ch0.lo_rate = new_lo_rate;
                end
                4'd1: begin
                    ch1.ca_freq_integrator = new_ca_freq_integrator;
                    ch1.lo_freq_integrator = new_lo_freq_integrator;
                    ch1.ca_rate = new_ca_rate;
                    ch1.lo_rate = new_lo_rate;
                end
            endcase
        end
    
        // Move to the next channel
        if (ch_to_track == 1) begin
            ch_to_track = 0;
        end else begin
            ch_to_track = ch_to_track + 1;
        end
    end
    
    int sav_num = 0;
    int fd;
    
    initial fd = $fopen("iq_sav.csv", "w");
    
    // Extract prompt I
    assign ip[0] = tracked_outs[0][56:43];
    assign ip[1] = tracked_outs[1][56:43];
    
    // Extract prompt Q
    assign qp[0] = tracked_outs[0][42:29];
    assign qp[1] = tracked_outs[1][42:29];
    
    always @(posedge epoch[1]) begin
        if(sav_num < 100) begin
            $fdisplay(fd, "%d,%d", ip[1], qp[1]);
            sav_num = sav_num + 1;
        end else begin
            $fclose(fd);
        end
    end
    
endmodule
