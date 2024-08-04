`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 06/16/2024 06:30:00 PM
// Design Name: 
// Module Name: boc11_channel
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
module boc11_channel
    #(
        parameter SAMPLE_FREQ = 16800000.0,
        parameter IF_FREQ = 3780000.0,
        parameter SV = 5'd28,
        parameter LO_DOP = -2106.0,
        parameter CA_SHIFT = 456.2)
    (
    input wire clk,
    input wire rst,
    input wire sample,
    output wire epoch_filt,
    output reg [108:1] tracked_outs
    );
    
    real ca_dop = LO_DOP/1575.42e6*6138000.0;
    reg [18:0] phase_offset = int'((SAMPLE_FREQ/125.0) - (CA_SHIFT*(SAMPLE_FREQ/250.0)/4092.0)) % int'(SAMPLE_FREQ/250.0);
    
    wire boc6_full;
    wire boc6_half;
    wire lo_full;
    wire lo_half;
    reg ca_en;
    
    reg unsigned [31:0] ca_rate = longint'((6138000.0 + ca_dop)*(2.0**32)/SAMPLE_FREQ);
    reg unsigned [31:0] lo_rate = longint'((IF_FREQ + LO_DOP)*(2.0**32)/SAMPLE_FREQ);
    
    reg signed [63:0] ca_freq_integrator = {ca_rate, 32'b0};
    reg signed [63:0] lo_freq_integrator = {lo_rate, 32'b0};
    
    wire ca_ve;
    reg ca_e, ca_p, ca_l, ca_vl;
    
    // Instantiate NCOs
    nco nco0 (ca_en, clk, ca_rate, boc6_full, boc6_half);       // BOC6 Code clock
    nco nco1 (1'b1, clk, lo_rate, lo_full, lo_half);            // LO clock
    
    // Delay CA generator to phase offset
    always @ (posedge clk) begin
        if (phase_offset) begin
            phase_offset = phase_offset - 1;
        end else begin
            ca_en = 1;
        end
    end
    
    // Generate boc1 clock
    reg [1:0] boc6to1 = 2'd2;
    reg [1:0] boc6to2 = 2'd2;
    reg boc1_full;
    reg boc1_half;
    reg dll_clk;
    always @ (posedge ca_en) boc6to2 = 2'd2;
    always @ (posedge boc6_full or negedge boc6_full) begin
        // Generate BOC1
        if (~boc6_full) begin
            if(boc6to1 == 2'd1) begin
                boc1_half = 1;
            end
        end else if (boc6to1 == 2'd2) begin
            boc6to1  = 2'd0;
            boc1_full = ~boc1_full;
            boc1_half = 0;
        end else begin
            boc6to1 = boc6to1 + 1;
        end
        
        // Generate clock for spacing codes
        if(boc6to2 == 2'd2) begin
            dll_clk = ~dll_clk;
            boc6to2 = 0;
        end else begin
            boc6to2 = boc6to2 + 1;
        end
    end
    
    // Generate CA codes
    always @ (posedge dll_clk or negedge dll_clk) begin
        ca_vl <= ca_l;
        ca_l <= ca_p;
        ca_p <= ca_e;
        ca_e <= ca_ve;
    end
    
    wire true_epoch;
    e1code e1code0 (1'b0, boc1_full, SV, ca_ve, true_epoch, epoch);
    
    // Synthesize local oscillator
    wire [3:0] lo_sin = 4'b1100;
    wire [3:0] lo_cos = 4'b0110;

    wire lo_i = lo_sin[{lo_full, lo_half}];
    wire lo_q = lo_cos[{lo_full, lo_half}];
    
    // Synthesize subcarrier
    wire boc11sc = boc1_full;
    wire boc61sc = boc6_full;
    
    reg dive, dqve, die, dqe, dip, dqp, dil, dql, divl, dqvl;
    reg signed [18:1] ive, qve, ie, qe, ip, qp, il, ql, ivl, qvl;

    always @ (posedge clk) begin
        // Mixers
        dive <= sample^ca_ve^lo_i^boc11sc; dqve <= sample^ca_ve^lo_q^boc11sc;
        die <= sample^ca_e^lo_i^boc61sc; dqe <= sample^ca_e^lo_q^boc61sc;
        dip <= sample^ca_p^lo_i^boc11sc; dqp <= sample^ca_p^lo_q^boc11sc;
        dil <= sample^ca_l^lo_i^boc61sc; dql <= sample^ca_l^lo_q^boc61sc;
        divl <= sample^ca_vl^lo_i^boc11sc; dqvl <= sample^ca_vl^lo_q^boc11sc;
        
        // Accumulators
        ive = ca_en ? (ive + {18{dive}} + {17'b0, ~dive}): 0;
        qve = ca_en ? (qve + {18{dqve}} + {17'b0, ~dqve}): 0;
        ie = ca_en ? (ie + {18{die}} + {17'b0, ~die}): 0;
        qe = ca_en ? (qe + {18{dqe}} + {17'b0, ~dqe}): 0;
        ip = ca_en ? (ip + {18{dip}} + {17'b0, ~dip}): 0;
        qp = ca_en ? (qp + {18{dqp}} + {17'b0, ~dqp}): 0;
        il = ca_en ? (il + {18{dil}} + {17'b0, ~dil}): 0;
        ql = ca_en ? (ql + {18{dql}} + {17'b0, ~dql}): 0;
        ivl = ca_en ? (ivl + {18{divl}} + {17'b0, ~divl}): 0;
        qvl = ca_en ? (qvl + {18{dqvl}} + {17'b0, ~dqvl}): 0;
    end
    
    // Filter epoch signal by ca_en so it doesn't trigger the
    // tracking logic when the ca generator isn't actually going.
    // Delay the ca_en signal to avoid timing problem.
    reg [1:0] ca_en_sync = 0;
    always_ff @(posedge clk, posedge rst) begin
        if(rst) ca_en_sync = 0;
        else ca_en_sync <= {ca_en_sync[0], ca_en};
    end
    assign epoch_filt = ca_en_sync[1] & true_epoch;
    
    reg signed [63:0] power_early;
    reg signed [63:0] power_late;
    reg signed [63:0] power_very_early;
    reg signed [63:0] power_very_late;
    reg signed [63:0] new_ca_rate_long;
    reg signed [63:0] new_lo_rate_long;
    reg signed [63:0] code_phase_err;
    reg signed [63:0] carrier_phase_err;
    reg signed [63:0] new_ca_freq_integrator;
    reg signed [63:0] new_lo_freq_integrator;
    reg [31:0] new_ca_rate;
    reg [31:0] new_lo_rate;
    reg signed [31:0] ch_power;
    
    always @ (posedge epoch_filt) begin
        // Delay locked loop error calculations
        power_very_early = (longint'(ive)**2)+(longint'(qve)**2);
        power_early = (longint'(ie)**2)+(longint'(qe)**2);
        power_late = (longint'(il)**2)+(longint'(ql)**2);
        power_very_late = (longint'(ivl)**2)+(longint'(qvl)**2);
        ch_power = (longint'(ip)**2)+(longint'(qp)**2);
        code_phase_err = power_early - power_late;
        
        // CA code tracking loop
        new_ca_freq_integrator = ca_freq_integrator + (code_phase_err <<< 12);
        new_ca_rate_long = ca_freq_integrator + (code_phase_err <<< 19);
        new_ca_rate = new_ca_rate_long[63:32];
        
        // Carrier tracking loop
        carrier_phase_err = ip*qp;
        new_lo_freq_integrator = lo_freq_integrator + (carrier_phase_err <<< 10);
        new_lo_rate_long = lo_freq_integrator + (carrier_phase_err <<< 20);
        new_lo_rate = new_lo_rate_long[63:32];
        
        ca_freq_integrator = new_ca_freq_integrator;
        lo_freq_integrator = new_lo_freq_integrator;
        ca_rate = new_ca_rate;
        lo_rate = new_lo_rate;
        
        // Save the final state of the accumulators
        tracked_outs <= {ie, qe, ip, qp, il, ql};
    
        // Reset accumulators for next epoch
        ive <= 0;
        qve <= 0;
        ie <= 0;
        qe <= 0;
        ip <= 0;
        qp <= 0;
        il <= 0;
        ql <= 0;
        ivl <= 0;
        qvl <= 0;
    end
    
    // Reset everything!
    always @ (posedge rst) begin
        // CA signals
        ca_en <= 0;
        ca_e <= 0;
        ca_p <= 0;
        ca_l <= 0;
        ca_vl <= 0;
       
        // BOC
        boc1_full <= 0;
        boc1_half <= 0;
        dll_clk <= 0;
        
        // Integrators
        dive <= 0;
        dqve <= 0;
        die <= 0;
        dqe <= 0;
        dip <= 0;
        dqp <= 0;
        dil <= 0;
        dql <= 0;
        divl <= 0;
        dqvl <= 0;
        ive <= 0;
        qve <= 0;
        ie <= 0;
        qe <= 0;
        ip <= 0;
        qp <= 0;
        il <= 0;
        ql <= 0;
        ivl <= 0;
        qvl <= 0;
        tracked_outs <= 108'b0;
        
    end
endmodule
