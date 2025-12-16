`timescale 1ns / 1ps

module axis_upcounter32 (
    // 1. Clock and Reset (Standard AXI names)
    input  wire          aclk,
    input  wire          aresetn,      // Active Low Reset

    // 2. AXI Stream Master Interface (Connects to FIFO/DMA)
    output reg  [31:0]   m_axis_tdata, // The counter value
    output wire [3:0]    m_axis_tkeep, // REQUIRED for DMA (Byte enable)
    output reg           m_axis_tlast, // End of packet
    output reg           m_axis_tvalid,// Data valid
    input  wire          m_axis_tready,// Back-pressure from FIFO

    // 3. User Configuration
    input  wire [31:0]   packet_size   // Previously 'cycle_limit'
);

    // Set TKEEP to all 1s (indicating all 4 bytes are valid)
    assign m_axis_tkeep = 4'b1111;

    always @(posedge aclk or negedge aresetn) begin
        // Reset Logic
        if (!aresetn) begin
            m_axis_tdata  <= 32'd0;
            m_axis_tlast  <= 1'b0;
            m_axis_tvalid <= 1'b0;
        end 
        else begin
            // Flow Control: Check if FIFO/DMA is ready to accept data
            if (m_axis_tready) begin
                
                m_axis_tvalid <= 1'b1; // Valid while running

                // Check for packet end (Packet Size - 2)
                if (m_axis_tdata == (packet_size - 2)) begin
                    m_axis_tdata <= m_axis_tdata + 1'b1;
                    m_axis_tlast <= 1'b1;       // Assert TLAST
                end 
                
                // Wrap around logic (Packet Size - 1)
                else if (m_axis_tdata == (packet_size - 1)) begin
                    m_axis_tdata <= 32'd0;      // Reset counter
                    m_axis_tlast <= 1'b0;       // De-assert TLAST
                end 
                
                // Normal Increment
                else begin
                    m_axis_tdata <= m_axis_tdata + 1'b1;
                    m_axis_tlast <= 1'b0;
                end
            end
            // If m_axis_tready is 0, we simply HOLD (Pause) values.
        end
    end
endmodule
