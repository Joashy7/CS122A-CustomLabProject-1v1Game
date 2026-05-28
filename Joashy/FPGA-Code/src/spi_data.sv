module spi_data (
    input wire spi_clk,
    input wire cs_n,       // active low
    input wire mosi,
 
    output reg [9:0] mem_waddr,
    output reg [15:0] mem_wdata,
    output reg mem_we
);

logic [15:0] shift_reg;
logic [3:0] bit_cnt;    // 0–15, bits within current 16-bit value
 
always_ff @(posedge spi_clk) begin
    if (cs_n) begin
        bit_cnt   <= 0;
        mem_we    <= 0;
        mem_waddr <= 0;
        shift_reg <= 0;
    end else begin
        shift_reg <= {shift_reg[14:0], mosi};
        mem_we  <= 0;
        if (bit_cnt == 15) begin
            mem_wdata <= {shift_reg[14:0], mosi};
            mem_we  <= 1;
            bit_cnt <= 0;
        end else begin
            bit_cnt <= bit_cnt + 1;
        end
        if (mem_we) begin
            mem_waddr <= mem_waddr + 1;
        end
    end
end
 
endmodule