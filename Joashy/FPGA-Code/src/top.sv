
`include "src/lcd.sv"
`include "src/dp_buffer.sv"
`include "src/spi_data.sv"

module top(
	input CLK,
	input SPI_SCK,
	input SPI_MOSI,
	input SPI_CS,

	output LCD_CLK,//LCD clock. Should be 8MHz
	output LCD_DEN,
	output logic [4:0] LCD_R,
	output logic [5:0] LCD_G,
	output logic [4:0] LCD_B
);
/** Logic */
reg [9:0] wpixel_addr;
reg [15:0] wpixel_data;
logic [9:0] rpixel_address;
logic [15:0] rpixel;
 
reg we;

assign LCD_CLK = CLK;

dp_buffer buffer(.clk(CLK), .we(we), .waddr(wpixel_addr), .wdata(wpixel_data), .raddr(rpixel_address), .rdata(rpixel));
lcd lcd(.pclk(CLK), .pixel(rpixel), .pixel_address(rpixel_address), .LCD_DE(LCD_DEN), .LCD_B(LCD_B), .LCD_G(LCD_G), .LCD_R(LCD_R));
spi_data spi(.spi_clk(SPI_SCK), .cs_n(SPI_CS), .mosi(SPI_MOSI), .mem_waddr(wpixel_addr), .mem_wdata(wpixel_data), .mem_we(we));


endmodule