//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/24/2023 10:53:54 AM
// Design Name: 
// Module Name: testing
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

module main (
    input clk,
    input sample_clk
    );
    
    integer file;
    integer f_ie;
    integer f_qe;
    integer f_ip;
    integer f_qp;
    integer f_il;
    integer f_ql;
    
    real lo_dop = -2106;
    real ca_shift = 456.2;
    
    real ca_dop = lo_dop/1575.42e6*1023000.0;
    reg [14:0] phase_offset = int'(20000 - (ca_shift*10000.0/1023.0)) % 10000;
    
    reg file_bit = 0; 
    reg [2:0] file_buf_idx = 0;
    reg [7:0] file_buf = 0;
    reg [2:0] save_idx = 0;
    integer save_num = 0;
    reg [7:0] ie_buf = 0;
    reg [7:0] qe_buf = 0;
    reg [7:0] ip_buf = 0;
    reg [7:0] qp_buf = 0;
    reg [7:0] il_buf = 0;
    reg [7:0] ql_buf = 0;
    
    initial begin
        file = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/gps.samples.1bit.I.fs10000.if2716.bin", "rb");
        f_ie = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/ie.csv", "w");
        f_qe = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/qe.csv", "w");
        f_ip = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/ip.csv", "w");
        f_qp = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/qp.csv", "w");
        f_il = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/il.csv", "w");
        f_ql = $fopen("C:/Users/griff/OneDrive/Documents/GNSS_SDR_Matlab/testing/ql.csv", "w");
    end
    
    wire clk_pll;
    wire clk_sample;
    wire clk_sample_half;
    wire ca_full;
    wire ca_half;
    wire lo_full;
    wire lo_half;
    wire epoch;
    reg ca_en = 0;
    
    reg [31:0] ca_rate = int'(real'(1023000.0 + ca_dop)/10000000.0*(2.0**32));
    reg [31:0] lo_rate = int'(real'(2716000.0 + lo_dop)/10000000.0*(2.0**32));
    
    reg signed [63:0] power_early = 0;
    reg signed [63:0]  power_late = 0;
    reg signed [63:0] code_phase_err = 0;
    reg signed [63:0] ca_freq_integrator = {ca_rate, 32'b0};
    reg [63:0] new_ca_rate;
    reg signed [63:0] carrier_phase_err = 0;
    reg signed [63:0] lo_freq_integrator = {lo_rate, 32'b0};
    reg [63:0] new_lo_rate;
    real theta;
    real last_theta;
    
    wire ca_e;
    reg ca_p, ca_l = 0;
    
    // Instantiate NCOs
    nco nco0 (ca_en, clk_sample, ca_rate, ca_full, ca_half);             // CA Code clock
    nco nco1 (1, clk_sample, lo_rate, lo_full, lo_half);                 // LO clock
    assign clk_sample = sample_clk;
    
    // Delay CA generator to phase offset
    always @ (posedge clk_sample) begin
        if (phase_offset) begin
            phase_offset = phase_offset - 1;
        end else begin
            ca_en = 1;
        end
    end
    
    // Generate CA codes
    always @ (posedge clk_sample) begin
        if (ca_full & ~ca_half) begin
            ca_l <= ca_p;
        end else if (~ca_full & ~ca_half) begin
            ca_p <= ca_e;
        end
    end
    
    cacode cacode0 (0, ca_full, 5'd29 - 1, ca_e, epoch);
    
    // Synthesize local oscillator
    wire [3:0] lo_sin = 4'b1100;
    wire [3:0] lo_cos = 4'b0110;

    wire lo_i = lo_sin[{lo_full, lo_half}];
    wire lo_q = lo_cos[{lo_full, lo_half}];
    
    reg sample;
    reg die = 0, dqe = 0, dip = 0, dqp = 0, dil = 0, dql = 0;
    reg signed [14:1] ie = 0, qe = 0, ip = 0, qp = 0, il = 0, ql = 0;

    always @ (posedge clk_sample) begin
    
        if(file_buf_idx == 0) begin
            $fread(file_buf, file);
        end
        
        sample = file_buf[file_buf_idx];
        file_buf_idx += 1;
        
        // Mixers
        die <= sample^ca_e^lo_i; dqe <= sample^ca_e^lo_q;
        dip <= sample^ca_p^lo_i; dqp <= sample^ca_p^lo_q;
        dil <= sample^ca_l^lo_i; dql <= sample^ca_l^lo_q;
        
        // Integrators
        ie = ca_en ? (ie + {14{die}} + {13'b0, ~die}): 0;
        qe = ca_en ? (qe + {14{dqe}} + {13'b0, ~dqe}): 0;
        ip = ca_en ? (ip + {14{dip}} + {13'b0, ~dip}): 0;
        qp = ca_en ? (qp + {14{dqp}} + {13'b0, ~dqp}): 0;
        il = ca_en ? (il + {14{dil}} + {13'b0, ~dil}): 0;
        ql = ca_en ? (ql + {14{dql}} + {13'b0, ~dql}): 0;
    end
    
    always @ (posedge epoch) begin
    
        if(phase_offset == 0 && save_num < 498) begin
            $fwrite(f_ie, "%d,", ie);
            $fwrite(f_qe, "%d,", qe);
            $fwrite(f_ip, "%d,", ip);
            $fwrite(f_qp, "%d,", qp);
            $fwrite(f_il, "%d,", il);
            $fwrite(f_ql, "%d,", ql);
            save_num += 1;
        end
        
        if(save_num == 498) begin
            $fclose(f_ie);
            $fclose(f_qe);
            $fclose(f_ip);
            $fclose(f_qp);
            $fclose(f_il);
            $fclose(f_ql);
        end
    
        // Delay locked loop error calculations
        power_early = (ie**2)+(qe**2);
        power_late = (il**2)+(ql**2);
        code_phase_err = power_early - power_late;
        
        // CA code tracking loop
        ca_freq_integrator = ca_freq_integrator + (code_phase_err <<< 11);
        new_ca_rate = (ca_freq_integrator + (code_phase_err <<< 23));
        ca_rate = new_ca_rate[63:32];
        
        // Carrier tracking loop
        carrier_phase_err = ip*qp;
        lo_freq_integrator = lo_freq_integrator + (carrier_phase_err <<< 20);
        new_lo_rate = lo_freq_integrator + (carrier_phase_err <<< 27);
        lo_rate = new_lo_rate[63:32];
        
        // Reset integrators for next epoch
        ie = 0;
        qe = 0;
        ip = 0;
        qp = 0;
        il = 0;
        ql = 0;
    end
endmodule
