`timescale 1ns / 1ps

module axis_upcounter32 (
    input  wire           aclk,
    input  wire           aresetn,
    output reg  [31:0]    m_axis_tdata,
    output wire [3:0]     m_axis_tkeep,
    output reg            m_axis_tlast,
    output reg            m_axis_tvalid,
    input  wire           m_axis_tready,
    input  wire [31:0]    packet_size
);

    assign m_axis_tkeep = 4'b1111;

    always @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            m_axis_tvalid <= 1'b0;
            m_axis_tdata  <= 32'd0;
            m_axis_tlast  <= 1'b0;
        end 
        else begin
            // 1. Always valid (Source behavior)
            m_axis_tvalid <= 1'b1;

            // 2. Handshake Check
            if (m_axis_tvalid && m_axis_tready) begin
                
                // EDGE CASE FIX: If packet_size is 1, we must handle it specially
                // or if we are at the end of a normal packet.
                
                // Check if we are currently at the LAST data word
                if (m_axis_tdata == packet_size - 1) begin
                    m_axis_tdata <= 32'd0; 
                    
                    // If the NEXT packet size is 1, TLAST must stay high!
                    // Otherwise, set it low.
                    if (packet_size == 1)
                        m_axis_tlast <= 1'b1;
                    else
                        m_axis_tlast <= 1'b0;
                end
                
                // Check if we are at the SECOND to last data word
                // (Pre-assert TLAST for the next cycle)
                else if (m_axis_tdata == packet_size - 2) begin
                    m_axis_tdata <= m_axis_tdata + 1;
                    m_axis_tlast <= 1'b1;
                end
                
                // Normal Increment
                else begin
                    m_axis_tdata <= m_axis_tdata + 1;
                    // Ensure TLAST is 0 unless packet_size is 1
                    if (packet_size == 1)
                        m_axis_tlast <= 1'b1;
                    else                 m_axis_tlast <= 1'b0;
                end
            end
        end
    end
endmodule
