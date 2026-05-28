
module lcd
(
    input logic  pclk,        // should get 8MHz clk
    input logic [15:0] pixel,
    output logic [9:0] pixel_address, 

    output logic LCD_DE,      // Display Enable

    output logic [4:0] LCD_B, // 5-bit blue color data
    output logic [5:0] LCD_G, // 6-bit green color data
    output logic [4:0] LCD_R  // 5-bit red color data
);

reg [9:0] x = 0, y = 0;

parameter x_max = 270;
parameter y_max = 270;

parameter x_buffer = 525;
parameter y_buffer = 285;

assign LCD_DE = (x < x_max) && (y < y_max);

assign pixel_address = ((y % 27) * 27) + (x % 27);

always_ff @( posedge pclk ) begin
    if (x < x_buffer) begin
        x <= x + 1;
    end else begin
        x <= 0;
        if (y < y_buffer) y <= y + 1; 
        else y <= 0;
    end
end

always @(*) begin 
    if (LCD_DE) begin
        LCD_R <= pixel[15:11];
        LCD_G <= pixel[10:5];
        LCD_B <= pixel[4:0];
    end else begin
        LCD_R <= 0;
        LCD_G <= 0;
        LCD_B <= 0;
    end
end

endmodule