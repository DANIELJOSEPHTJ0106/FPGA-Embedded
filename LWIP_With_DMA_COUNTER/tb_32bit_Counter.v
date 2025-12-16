`timescale 1ns / 1ps

module tb_axis_upcounter32;

    // 1. Declare Signals
    reg aclk;
    reg aresetn;
    reg m_axis_tready;
    reg [31:0] packet_size;

    wire [31:0] m_axis_tdata;
    wire [3:0]  m_axis_tkeep;
    wire        m_axis_tlast;
    wire        m_axis_tvalid;

    // 2. Instantiate the Counter (UUT)
    axis_upcounter32 uut (
        .aclk(aclk), 
        .aresetn(aresetn), 
        .m_axis_tdata(m_axis_tdata), 
        .m_axis_tkeep(m_axis_tkeep), 
        .m_axis_tlast(m_axis_tlast), 
        .m_axis_tvalid(m_axis_tvalid), 
        .m_axis_tready(m_axis_tready), 
        .packet_size(packet_size)
    );

    // 3. Clock Generation (100 MHz)
    initial begin
        aclk = 0;
        forever #5 aclk = ~aclk; 
    end

    // 4. Stimulus (The Important Part)
    initial begin
        // Initialize Inputs
        aresetn = 0;
        packet_size = 32'd1024; // Set packet size to 10
        m_axis_tready = 1;    // <--- FIX: Keep this 1 forever!

        // Apply Reset
        #100;
        @(negedge aclk);
        aresetn = 1;

        // Run simulation for enough time to see multiple loops
        #1000; 
        
        // Stop
        $finish;
    end

endmodule
