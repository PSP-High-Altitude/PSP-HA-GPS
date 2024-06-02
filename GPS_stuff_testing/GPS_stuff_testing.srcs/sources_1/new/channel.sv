//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 05/28/2024 09:06:22 PM
// Design Name: 
// Module Name: channel
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

module channel
    #(
        parameter SAMPLE_FREQ = 16800000.0,
        parameter IF_FREQ = 3780000.0,
        parameter SV = 5'd28,
        parameter LO_DOP = -2106.0,
        parameter CA_SHIFT = 456.2)
    (
    input clk,
    input wire sample,
    output wire epoch_filt,
    output reg [84:1] tracked_outs
    );
    
    real ca_dop = LO_DOP/1575.42e6*1023000.0;
    reg [14:0] phase_offset = int'((SAMPLE_FREQ/500.0) - (CA_SHIFT*(SAMPLE_FREQ/1000.0)/1023.0)) % int'(SAMPLE_FREQ/1000.0);
    
    wire ca_full;
    wire ca_half;
    wire lo_full;
    wire lo_half;
    reg ca_en = 0;
    
    reg [31:0] ca_rate = int'(real'(1023000.0 + ca_dop)/SAMPLE_FREQ*(2.0**32));
    reg [31:0] lo_rate = int'(real'(IF_FREQ + LO_DOP)/SAMPLE_FREQ*(2.0**32));
    
    reg signed [63:0] ca_freq_integrator = {ca_rate, 32'b0};
    reg signed [63:0] lo_freq_integrator = {lo_rate, 32'b0};
    real theta;
    real last_theta;
    
    wire ca_e;
    reg ca_p, ca_l = 0;
    
    // Instantiate NCOs
    nco nco0 (ca_en, clk, ca_rate, ca_full, ca_half);           // CA Code clock
    nco nco1 (1'b1, clk, lo_rate, lo_full, lo_half);            // LO clock
    
    // Delay CA generator to phase offset
    always @ (posedge clk) begin
        if (phase_offset) begin
            phase_offset = phase_offset - 1;
        end else begin
            ca_en = 1;
        end
    end
    
    // Generate CA codes
    always @ (posedge clk) begin
        if (ca_full & ~ca_half) begin
            ca_l <= ca_p;
        end else if (~ca_full & ~ca_half) begin
            ca_p <= ca_e;
        end
    end
    
    cacode cacode0 (1'b0, ca_full, SV, ca_e, epoch);
    
    // Synthesize local oscillator
    wire [3:0] lo_sin = 4'b1100;
    wire [3:0] lo_cos = 4'b0110;

    wire lo_i = lo_sin[{lo_full, lo_half}];
    wire lo_q = lo_cos[{lo_full, lo_half}];
    
    reg die = 0, dqe = 0, dip = 0, dqp = 0, dil = 0, dql = 0;
    reg signed [14:1] ie = 0, qe = 0, ip = 0, qp = 0, il = 0, ql = 0;

    always @ (posedge clk) begin
        // Mixers
        die <= sample^ca_e^lo_i; dqe <= sample^ca_e^lo_q;
        dip <= sample^ca_p^lo_i; dqp <= sample^ca_p^lo_q;
        dil <= sample^ca_l^lo_i; dql <= sample^ca_l^lo_q;
        
        // Accumulators
        ie = ca_en ? (ie + {14{die}} + {13'b0, ~die}): 0;
        qe = ca_en ? (qe + {14{dqe}} + {13'b0, ~dqe}): 0;
        ip = ca_en ? (ip + {14{dip}} + {13'b0, ~dip}): 0;
        qp = ca_en ? (qp + {14{dqp}} + {13'b0, ~dqp}): 0;
        il = ca_en ? (il + {14{dil}} + {13'b0, ~dil}): 0;
        ql = ca_en ? (ql + {14{dql}} + {13'b0, ~dql}): 0;
    end
    
    // Filter epoch signal by ca_en so it doesn't trigger the
    // tracking logic when the ca generator isn't actually going.
    // Delay the ca_en signal to avoid timing problem.
    reg [1:0] ca_en_sync = 0;
    always_ff @(posedge clk) begin
        ca_en_sync <= {ca_en_sync[0], ca_en};
    end
    assign epoch_filt = ca_en_sync[1] & epoch;
    
    always @ (posedge epoch) begin
        // Save the final state of the accumulators
        tracked_outs <= {ie, qe, ip, qp, il, ql};
    
        // Reset accumulators for next epoch
        ie <= 0;
        qe <= 0;
        ip <= 0;
        qp <= 0;
        il <= 0;
        ql <= 0;
    end
endmodule